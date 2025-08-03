/**
 * Enhanced Scene State Manager
 * 
 * This utility provides functions to save and load the exact scene state,
 * including all function values, integrator states, generator RNG states,
 * and other runtime data that the basic scene storage doesn't capture.
 */

const ENHANCED_STATE_PREFIX = 'lux_enhanced_state_';
const ENHANCED_STATE_VERSION = '2.0';

export class EnhancedSceneStateManager {
    
    /**
     * Export the current scene state as JSON
     * @returns {string} JSON string containing the complete scene state
     */
    static exportCurrentState() {
        if (!window.module) {
            throw new Error('WebAssembly module not loaded');
        }
        
        try {
            const stateJson = window.module.export_scene_state_json();
            return stateJson;
        } catch (error) {
            console.error('Error exporting scene state:', error);
            throw error;
        }
    }
    
    /**
     * Import a scene state from JSON
     * @param {string} stateJson - JSON string containing scene state
     * @returns {boolean} Success status
     */
    static importState(stateJson) {
        if (!window.module) {
            throw new Error('WebAssembly module not loaded');
        }
        
        try {
            const success = window.module.import_scene_state_json(stateJson);
            return success;
        } catch (error) {
            console.error('Error importing scene state:', error);
            throw error;
        }
    }
    
    /**
     * Save scene state to localStorage with metadata
     * @param {string} saveName - Name for the saved state
     * @param {string} description - Optional description
     * @returns {Object} Save result with metadata
     */
    static saveStateToStorage(saveName, description = '') {
        try {
            const stateJson = this.exportCurrentState();
            const metadata = JSON.parse(window.module.get_scene_state_metadata());
            
            const saveData = {
                version: ENHANCED_STATE_VERSION,
                timestamp: Date.now(),
                saveName,
                description,
                sceneName: metadata.scene_name,
                metadata,
                state: stateJson
            };
            
            const storageKey = `${ENHANCED_STATE_PREFIX}${saveName}`;
            localStorage.setItem(storageKey, JSON.stringify(saveData));
            
            console.log(`Enhanced scene state saved: ${saveName}`);
            return {
                success: true,
                saveName,
                metadata,
                storageKey
            };
        } catch (error) {
            console.error('Error saving enhanced scene state:', error);
            return {
                success: false,
                error: error.message
            };
        }
    }
    
    /**
     * Load scene state from localStorage
     * @param {string} saveName - Name of the saved state
     * @returns {Object} Load result
     */
    static loadStateFromStorage(saveName) {
        try {
            const storageKey = `${ENHANCED_STATE_PREFIX}${saveName}`;
            const data = localStorage.getItem(storageKey);
            
            if (!data) {
                return {
                    success: false,
                    error: `No saved state found: ${saveName}`
                };
            }
            
            const saveData = JSON.parse(data);
            
            // Version check
            if (saveData.version !== ENHANCED_STATE_VERSION) {
                console.warn(`State version mismatch: ${saveData.version} vs ${ENHANCED_STATE_VERSION}`);
            }
            
            // Import the state
            const success = this.importState(saveData.state);
            
            return {
                success,
                saveName,
                metadata: saveData.metadata,
                description: saveData.description,
                timestamp: saveData.timestamp
            };
        } catch (error) {
            console.error('Error loading enhanced scene state:', error);
            return {
                success: false,
                error: error.message
            };
        }
    }
    
    /**
     * List all saved enhanced states
     * @returns {Array} Array of saved state info objects
     */
    static listSavedStates() {
        const states = [];
        
        try {
            for (let i = 0; i < localStorage.length; i++) {
                const key = localStorage.key(i);
                if (key && key.startsWith(ENHANCED_STATE_PREFIX)) {
                    const saveName = key.replace(ENHANCED_STATE_PREFIX, '');
                    const data = localStorage.getItem(key);
                    
                    if (data) {
                        try {
                            const saveData = JSON.parse(data);
                            states.push({
                                saveName,
                                description: saveData.description || '',
                                sceneName: saveData.sceneName || 'Unknown',
                                timestamp: saveData.timestamp,
                                metadata: saveData.metadata,
                                version: saveData.version
                            });
                        } catch (parseError) {
                            console.warn(`Failed to parse saved state: ${saveName}`);
                        }
                    }
                }
            }
        } catch (error) {
            console.error('Error listing saved states:', error);
        }
        
        return states.sort((a, b) => b.timestamp - a.timestamp);
    }
    
    /**
     * Delete a saved state
     * @param {string} saveName - Name of the state to delete
     * @returns {boolean} Success status
     */
    static deleteSavedState(saveName) {
        try {
            const storageKey = `${ENHANCED_STATE_PREFIX}${saveName}`;
            localStorage.removeItem(storageKey);
            console.log(`Enhanced scene state deleted: ${saveName}`);
            return true;
        } catch (error) {
            console.error('Error deleting enhanced scene state:', error);
            return false;
        }
    }
    
    /**
     * Get current scene state metadata
     * @returns {Object} Metadata about current scene state
     */
    static getCurrentStateMetadata() {
        if (!window.module) {
            return null;
        }
        
        try {
            const metadataJson = window.module.get_scene_state_metadata();
            return JSON.parse(metadataJson);
        } catch (error) {
            console.error('Error getting scene metadata:', error);
            return null;
        }
    }
    
    /**
     * Export current state to a downloadable file
     * @param {string} filename - Name for the downloaded file
     */
    static exportToFile(filename = null) {
        try {
            const stateJson = this.exportCurrentState();
            const metadata = this.getCurrentStateMetadata();
            
            if (!filename) {
                const sceneName = metadata?.scene_name || 'scene';
                const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
                filename = `${sceneName}_state_${timestamp}.json`;
            }
            
            const blob = new Blob([stateJson], { type: 'application/json' });
            const url = URL.createObjectURL(blob);
            
            const a = document.createElement('a');
            a.href = url;
            a.download = filename;
            document.body.appendChild(a);
            a.click();
            document.body.removeChild(a);
            URL.revokeObjectURL(url);
            
            console.log(`Scene state exported to file: ${filename}`);
        } catch (error) {
            console.error('Error exporting to file:', error);
            throw error;
        }
    }
    
    /**
     * Import state from a file
     * @param {File} file - File object to import from
     * @returns {Promise<Object>} Import result
     */
    static async importFromFile(file) {
        return new Promise((resolve, reject) => {
            const reader = new FileReader();
            
            reader.onload = (e) => {
                try {
                    const stateJson = e.target.result;
                    const success = this.importState(stateJson);
                    
                    resolve({
                        success,
                        filename: file.name,
                        size: file.size
                    });
                } catch (error) {
                    reject(error);
                }
            };
            
            reader.onerror = () => {
                reject(new Error('Failed to read file'));
            };
            
            reader.readAsText(file);
        });
    }
    
    /**
     * Create a savestate with automatic naming
     * @param {string} baseName - Base name for the savestate
     * @returns {string} Generated filename
     */
    static createSavestate(baseName = 'savestate') {
        if (!window.module) {
            throw new Error('WebAssembly module not loaded');
        }
        
        try {
            const filename = window.module.create_scene_savestate(baseName);
            console.log(`Savestate created: ${filename}`);
            return filename;
        } catch (error) {
            console.error('Error creating savestate:', error);
            throw error;
        }
    }
    
    /**
     * Compare two scene states
     * @param {string} stateJson1 - First state JSON
     * @param {string} stateJson2 - Second state JSON
     * @returns {Object} Comparison result
     */
    static compareStates(stateJson1, stateJson2) {
        try {
            const state1 = JSON.parse(stateJson1);
            const state2 = JSON.parse(stateJson2);
            
            const differences = [];
            const similarities = [];
            
            // Compare functions
            if (state1.functions && state2.functions) {
                const funcNames1 = new Set(state1.functions.map(f => f.name));
                const funcNames2 = new Set(state2.functions.map(f => f.name));
                
                // Find common functions
                const commonFuncs = [...funcNames1].filter(name => funcNames2.has(name));
                
                for (const funcName of commonFuncs) {
                    const func1 = state1.functions.find(f => f.name === funcName);
                    const func2 = state2.functions.find(f => f.name === funcName);
                    
                    if (JSON.stringify(func1) !== JSON.stringify(func2)) {
                        differences.push({
                            type: 'function',
                            name: funcName,
                            value1: func1,
                            value2: func2
                        });
                    } else {
                        similarities.push({
                            type: 'function',
                            name: funcName,
                            value: func1
                        });
                    }
                }
            }
            
            return {
                differences,
                similarities,
                totalDifferences: differences.length,
                totalSimilarities: similarities.length
            };
        } catch (error) {
            console.error('Error comparing states:', error);
            throw error;
        }
    }
}

export default EnhancedSceneStateManager; 