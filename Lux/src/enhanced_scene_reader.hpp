#ifndef __ENHANCED_SCENE_READER_HPP
#define __ENHANCED_SCENE_READER_HPP

#include "scene.hpp"
#include "scene_io.hpp"
#include "json.hpp"
#include <fstream>
#include <sstream>
#include <chrono>
#include <set>
#include <vector>

#define DEBUG( msg ) { std::string debug_msg = msg; std::cout << debug_msg << std::endl; }
#define ERROR( msg ) throw std::runtime_error( msg );

using json = nlohmann::json;
using string = std::string;

class enhanced_scene_reader {
private:
    scene& s;

public:
    // Constructor for reading from file
    enhanced_scene_reader(scene& s_init, const std::string& filename) : s(s_init) {
        DEBUG("Enhanced Scene Reader constructor");

        std::fstream in_stream(filename);
        json j;
        try {
            j = json::parse(in_stream);
        } catch(json::parse_error& ex) {
            ERROR("Error parsing JSON: " + std::to_string(ex.byte));
        }

        DEBUG("Scene file parsed into json object")

        if (j.contains("name")) j["name"].get_to(s.name);
        else s.name = "Unnamed";

        if (j.contains("time_interval")) j["time_interval"].get_to(s.time_interval);
        if (j.contains("liveCamera")) j["liveCamera"].get_to(s.liveCamera);
        else s.liveCamera = false;

        if (j.contains("images")) {
            for(const auto& img_data: j["images"]) {
                read_image(img_data);
            }
            DEBUG("Images read successfully");
        }

        if (j.contains("elements")) {
            for(auto& jelem: j["elements"]) read_element(jelem);
            DEBUG("Elements read successfully");
        }

        setup_enhanced_functions();
        DEBUG("Enhanced functions setup successfully");

        if (j.contains("functions")) {
            for(auto& jfunc: j["functions"]) read_function_enhanced(jfunc);
            DEBUG("Enhanced functions read successfully");
        }

        if (j.contains("clusters")) {
            for(auto& jcluster: j["clusters"]) read_cluster(jcluster);
            DEBUG("Clusters read successfully");
        }

        if (j.contains("effects")) {
            for(auto& jeffect: j["effects"]) read_effect(jeffect);
            DEBUG("Effects read successfully");
        }

        if (j.contains("queues")) {
            for(auto& q: j["queues"]) read_queue(q);
            DEBUG("Queues read successfully");
        }

        if (j.contains("widget_groups")) {
            for(auto& wg: j["widget_groups"]) read_widget_group(wg);
            DEBUG("Widget groups read successfully");
        }

        finalize_scene_setup();
        DEBUG("Scene setup finalized successfully");
    }

    // Static method for state management without file reading
    static json export_scene_state(scene& s) {
        json scene_state;
        
        // Basic scene info
        scene_state["name"] = s.name;
        if(s.time_interval != 1.0f/60.0f) { // Only include if non-default
            scene_state["time_interval"] = s.time_interval;
        }
        scene_state["liveCamera"] = s.liveCamera;
        
        // Export images
        if(!s.buffers.empty()) {
            json images_array = json::array();
            for(const auto& [name, buffer] : s.buffers) {
                json img_data;
                img_data["name"] = name;
                // For now, mark all as uimage type
                img_data["type"] = "uimage";
                img_data["filename"] = "lux_files/" + name + ".jpg"; // Placeholder
                images_array.push_back(img_data);
            }
            scene_state["images"] = images_array;
        }
        
        // Export all functions with their current state
        if(!s.functions.empty()) {
            json functions_array = json::array();
            for(const auto& [name, func] : s.functions) {
                json func_data = serialize_function_state_static(name, func);
                if(!func_data.empty()) {
                    functions_array.push_back(func_data);
                }
            }
            scene_state["functions"] = functions_array;
        }
        
        // Add metadata about state capture
        scene_state["_state_metadata"] = {
            {"capture_time", std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count()},
            {"total_functions", s.functions.size()},
            {"version", "enhanced_v1.0"}
        };
        
        return scene_state;
    }

    // Export current scene state as JSON
    json export_exact_scene_state() const {
        json scene_state;
        
        // Basic scene info
        scene_state["name"] = s.name;
        if(s.time_interval != 1.0f/60.0f) { // Only include if non-default
            scene_state["time_interval"] = s.time_interval;
        }
        scene_state["liveCamera"] = s.liveCamera;
        
        // Export images
        if(!s.buffers.empty()) {
            json images_array = json::array();
            for(const auto& [name, buffer] : s.buffers) {
                json img_data;
                img_data["name"] = name;
                // For now, mark all as uimage type
                img_data["type"] = "uimage";
                img_data["filename"] = "lux_files/" + name + ".jpg"; // Placeholder
                images_array.push_back(img_data);
            }
            scene_state["images"] = images_array;
        }
        
        // Export all functions with their current state
        if(!s.functions.empty()) {
            json functions_array = json::array();
            for(const auto& [name, func] : s.functions) {
                json func_data = serialize_function_state(name, func);
                if(!func_data.empty()) {
                    functions_array.push_back(func_data);
                }
            }
            scene_state["functions"] = functions_array;
        }
        
        // Add metadata about state capture
        scene_state["_state_metadata"] = {
            {"capture_time", std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count()},
            {"total_functions", s.functions.size()},
            {"version", "enhanced_v1.0"}
        };
        
        return scene_state;
    }

    // Static method for importing scene state
    static bool import_scene_state(scene& s, const json& scene_data) {
        try {
            int restored_count = 0;
            int total_count = 0;
            
            if(scene_data.contains("functions")) {
                for(const auto& func_data : scene_data["functions"]) {
                    if(!func_data.contains("name")) continue;
                    
                    std::string name = func_data["name"];
                    total_count++;
                    
                    if(s.functions.contains(name)) {
                        bool restored = deserialize_function_state_static(name, s.functions[name], func_data);
                        if(restored) restored_count++;
                    }
                }
            }
            
            DEBUG("Restored " + std::to_string(restored_count) + "/" + std::to_string(total_count) + " function states");
            return restored_count > 0;
            
        } catch(const std::exception& e) {
            ERROR("Failed to import scene state: " + std::string(e.what()));
            return false;
        }
    }

    // Import scene state from JSON
    bool import_exact_scene_state(const json& scene_data) {
        try {
            int restored_count = 0;
            int total_count = 0;
            
            if(scene_data.contains("functions")) {
                for(const auto& func_data : scene_data["functions"]) {
                    if(!func_data.contains("name")) continue;
                    
                    std::string name = func_data["name"];
                    total_count++;
                    
                    if(s.functions.contains(name)) {
                        bool restored = deserialize_function_state(name, s.functions[name], func_data);
                        if(restored) restored_count++;
                    }
                }
            }
            
            DEBUG("Restored " + std::to_string(restored_count) + "/" + std::to_string(total_count) + " function states");
            return restored_count > 0;
            
        } catch(const std::exception& e) {
            ERROR("Failed to import scene state: " + std::string(e.what()));
            return false;
        }
    }

private:
    // Static serialization method
    static json serialize_function_state_static(const std::string& name, const any_function& func) {
        json func_data;
        func_data["name"] = name;
        
        // Handle different function types
        if (std::holds_alternative<any_fn<float>>(func)) {
            const auto& float_fn = std::get<any_fn<float>>(func);
            if (std::holds_alternative<std::shared_ptr<slider_float>>(float_fn.any_fn_ptr)) {
                const auto& slider = std::get<std::shared_ptr<slider_float>>(float_fn.any_fn_ptr);
                func_data["type"] = "slider_float";
                func_data["value"] = *slider->value;
                func_data["min"] = slider->min;
                func_data["max"] = slider->max;
                func_data["step"] = slider->step;
                func_data["label"] = slider->label;
                func_data["description"] = slider->description;
            }
        } else if (std::holds_alternative<any_fn<int>>(func)) {
            const auto& int_fn = std::get<any_fn<int>>(func);
            if (std::holds_alternative<std::shared_ptr<slider_int>>(int_fn.any_fn_ptr)) {
                const auto& slider = std::get<std::shared_ptr<slider_int>>(int_fn.any_fn_ptr);
                func_data["type"] = "slider_int";
                func_data["value"] = *slider->value;
                func_data["min"] = slider->min;
                func_data["max"] = slider->max;
                func_data["step"] = slider->step;
                func_data["label"] = slider->label;
                func_data["description"] = slider->description;
            } else if (std::holds_alternative<std::shared_ptr<menu_int>>(int_fn.any_fn_ptr)) {
                const auto& menu = std::get<std::shared_ptr<menu_int>>(int_fn.any_fn_ptr);
                func_data["type"] = "menu_int";
                func_data["choice"] = *menu->choice;
                func_data["items"] = menu->items;
                func_data["label"] = menu->label;
                func_data["description"] = menu->description;
            }
        } else if (std::holds_alternative<any_fn<bool>>(func)) {
            const auto& bool_fn = std::get<any_fn<bool>>(func);
            if (std::holds_alternative<std::shared_ptr<switch_fn>>(bool_fn.any_fn_ptr)) {
                const auto& switch_val = std::get<std::shared_ptr<switch_fn>>(bool_fn.any_fn_ptr);
                func_data["type"] = "switch_fn";
                func_data["value"] = *switch_val->value;
                func_data["label"] = switch_val->label;
                func_data["description"] = switch_val->description;
            }
        } else if (std::holds_alternative<any_fn<std::string>>(func)) {
            const auto& string_fn = std::get<any_fn<std::string>>(func);
            if (std::holds_alternative<std::shared_ptr<menu_string>>(string_fn.any_fn_ptr)) {
                const auto& menu = std::get<std::shared_ptr<menu_string>>(string_fn.any_fn_ptr);
                func_data["type"] = "menu_string";
                func_data["choice"] = *menu->choice;
                func_data["items"] = menu->items;
                func_data["label"] = menu->label;
                func_data["description"] = menu->description;
            }
        }
        
        return func_data;
    }

    // Static deserialization method
    static bool deserialize_function_state_static(const std::string& name, any_function& func, const json& func_data) {
        try {
            std::string type = func_data.value("type", "");
            
            if (type == "slider_float" && std::holds_alternative<any_fn<float>>(func)) {
                auto& float_fn = std::get<any_fn<float>>(func);
                if (std::holds_alternative<std::shared_ptr<slider_float>>(float_fn.any_fn_ptr)) {
                    auto slider = std::get<std::shared_ptr<slider_float>>(float_fn.any_fn_ptr);
                    if (func_data.contains("value")) {
                        *slider->value = func_data["value"].get<float>();
                        return true;
                    }
                }
            } else if (type == "slider_int" && std::holds_alternative<any_fn<int>>(func)) {
                auto& int_fn = std::get<any_fn<int>>(func);
                if (std::holds_alternative<std::shared_ptr<slider_int>>(int_fn.any_fn_ptr)) {
                    auto slider = std::get<std::shared_ptr<slider_int>>(int_fn.any_fn_ptr);
                    if (func_data.contains("value")) {
                        *slider->value = func_data["value"].get<int>();
                        return true;
                    }
                }
            } else if (type == "menu_int" && std::holds_alternative<any_fn<int>>(func)) {
                auto& int_fn = std::get<any_fn<int>>(func);
                if (std::holds_alternative<std::shared_ptr<menu_int>>(int_fn.any_fn_ptr)) {
                    auto menu = std::get<std::shared_ptr<menu_int>>(int_fn.any_fn_ptr);
                    if (func_data.contains("choice")) {
                        *menu->choice = func_data["choice"].get<int>();
                        return true;
                    }
                }
            } else if (type == "switch_fn" && std::holds_alternative<any_fn<bool>>(func)) {
                auto& bool_fn = std::get<any_fn<bool>>(func);
                if (std::holds_alternative<std::shared_ptr<switch_fn>>(bool_fn.any_fn_ptr)) {
                    auto switch_val = std::get<std::shared_ptr<switch_fn>>(bool_fn.any_fn_ptr);
                    if (func_data.contains("value")) {
                        *switch_val->value = func_data["value"].get<bool>();
                        return true;
                    }
                }
            } else if (type == "menu_string" && std::holds_alternative<any_fn<std::string>>(func)) {
                auto& string_fn = std::get<any_fn<std::string>>(func);
                if (std::holds_alternative<std::shared_ptr<menu_string>>(string_fn.any_fn_ptr)) {
                    auto menu = std::get<std::shared_ptr<menu_string>>(string_fn.any_fn_ptr);
                    if (func_data.contains("choice")) {
                        *menu->choice = func_data["choice"].get<std::string>();
                        return true;
                    }
                }
            }
            
            return false;
        } catch(const std::exception& e) {
            DEBUG("Error deserializing function " + name + ": " + e.what());
            return false;
        }
    }

    // Serialize function state based on type
    json serialize_function_state(const std::string& name, const any_function& func) const {
        json func_data;
        func_data["name"] = name;
        
        // Handle different function types
        if (std::holds_alternative<any_fn<float>>(func)) {
            auto& float_fn = std::get<any_fn<float>>(func);
            if (std::holds_alternative<std::shared_ptr<slider_float>>(float_fn.any_fn_ptr)) {
                auto slider = std::get<std::shared_ptr<slider_float>>(float_fn.any_fn_ptr);
                func_data["type"] = "slider_float";
                func_data["value"] = *slider->value;
                func_data["min"] = slider->min;
                func_data["max"] = slider->max;
                func_data["step"] = slider->step;
                func_data["label"] = slider->label;
                func_data["description"] = slider->description;
            }
        } else if (std::holds_alternative<any_fn<int>>(func)) {
            auto& int_fn = std::get<any_fn<int>>(func);
            if (std::holds_alternative<std::shared_ptr<slider_int>>(int_fn.any_fn_ptr)) {
                auto slider = std::get<std::shared_ptr<slider_int>>(int_fn.any_fn_ptr);
                func_data["type"] = "slider_int";
                func_data["value"] = *slider->value;
                func_data["min"] = slider->min;
                func_data["max"] = slider->max;
                func_data["step"] = slider->step;
                func_data["label"] = slider->label;
                func_data["description"] = slider->description;
            } else if (std::holds_alternative<std::shared_ptr<menu_int>>(int_fn.any_fn_ptr)) {
                auto menu = std::get<std::shared_ptr<menu_int>>(int_fn.any_fn_ptr);
                func_data["type"] = "menu_int";
                func_data["choice"] = *menu->choice;
                func_data["items"] = menu->items;
                func_data["label"] = menu->label;
                func_data["description"] = menu->description;
            }
        } else if (std::holds_alternative<any_fn<bool>>(func)) {
            auto& bool_fn = std::get<any_fn<bool>>(func);
            if (std::holds_alternative<std::shared_ptr<switch_fn>>(bool_fn.any_fn_ptr)) {
                auto switch_val = std::get<std::shared_ptr<switch_fn>>(bool_fn.any_fn_ptr);
                func_data["type"] = "switch_fn";
                func_data["value"] = *switch_val->value;
                func_data["label"] = switch_val->label;
                func_data["description"] = switch_val->description;
            }
        } else if (std::holds_alternative<any_fn<std::string>>(func)) {
            auto& string_fn = std::get<any_fn<std::string>>(func);
            if (std::holds_alternative<std::shared_ptr<menu_string>>(string_fn.any_fn_ptr)) {
                auto menu = std::get<std::shared_ptr<menu_string>>(string_fn.any_fn_ptr);
                func_data["type"] = "menu_string";
                func_data["choice"] = *menu->choice;
                func_data["items"] = menu->items;
                func_data["label"] = menu->label;
                func_data["description"] = menu->description;
            }
        }
        
        return func_data;
    }

    // Deserialize function state
    bool deserialize_function_state(const std::string& name, any_function& func, const json& func_data) {
        try {
            std::string type = func_data.value("type", "");
            
            if (type == "slider_float" && std::holds_alternative<any_fn<float>>(func)) {
                auto& float_fn = std::get<any_fn<float>>(func);
                if (std::holds_alternative<std::shared_ptr<slider_float>>(float_fn.any_fn_ptr)) {
                    auto slider = std::get<std::shared_ptr<slider_float>>(float_fn.any_fn_ptr);
                    if (func_data.contains("value")) {
                        slider->value = func_data["value"].get<float>();
                        return true;
                    }
                }
            } else if (type == "slider_int" && std::holds_alternative<any_fn<int>>(func)) {
                auto& int_fn = std::get<any_fn<int>>(func);
                if (std::holds_alternative<std::shared_ptr<slider_int>>(int_fn.any_fn_ptr)) {
                    auto slider = std::get<std::shared_ptr<slider_int>>(int_fn.any_fn_ptr);
                    if (func_data.contains("value")) {
                        slider->value = func_data["value"].get<int>();
                        return true;
                    }
                }
            } else if (type == "menu_int" && std::holds_alternative<any_fn<int>>(func)) {
                auto& int_fn = std::get<any_fn<int>>(func);
                if (std::holds_alternative<std::shared_ptr<menu_int>>(int_fn.any_fn_ptr)) {
                    auto menu = std::get<std::shared_ptr<menu_int>>(int_fn.any_fn_ptr);
                    if (func_data.contains("choice")) {
                        menu->choice = func_data["choice"].get<int>();
                        return true;
                    }
                }
            } else if (type == "switch_fn" && std::holds_alternative<any_fn<bool>>(func)) {
                auto& bool_fn = std::get<any_fn<bool>>(func);
                if (std::holds_alternative<std::shared_ptr<switch_fn>>(bool_fn.any_fn_ptr)) {
                    auto switch_val = std::get<std::shared_ptr<switch_fn>>(bool_fn.any_fn_ptr);
                    if (func_data.contains("value")) {
                        switch_val->value = func_data["value"].get<bool>();
                        return true;
                    }
                }
            } else if (type == "menu_string" && std::holds_alternative<any_fn<std::string>>(func)) {
                auto& string_fn = std::get<any_fn<std::string>>(func);
                if (std::holds_alternative<std::shared_ptr<menu_string>>(string_fn.any_fn_ptr)) {
                    auto menu = std::get<std::shared_ptr<menu_string>>(string_fn.any_fn_ptr);
                    if (func_data.contains("choice")) {
                        menu->choice = func_data["choice"].get<std::string>();
                        return true;
                    }
                }
            }
            
            return false;
        } catch(const std::exception& e) {
            DEBUG("Error deserializing function " + name + ": " + e.what());
            return false;
        }
    }

    // Setup enhanced functions (placeholder for now)
    void setup_enhanced_functions() {
        // This would initialize enhanced function types if needed
    }

    // Read function with enhanced capabilities
    void read_function_enhanced(const json& j) {
        // Use the existing scene_reader's read_function for now
        scene_reader reader(s, "");
        reader.read_function(j);
    }

    // Finalize scene setup
    void finalize_scene_setup() {
        // Any final setup needed
    }

    // Placeholder methods for compatibility
    void read_image(const json& j) {
        scene_reader reader(s, "");
        reader.read_image(j);
    }

    void read_element(const json& j) {
        scene_reader reader(s, "");
        reader.read_element(j);
    }

    void read_cluster(const json& j) {
        scene_reader reader(s, "");
        reader.read_cluster(j);
    }

    void read_effect(const json& j) {
        scene_reader reader(s, "");
        reader.read_effect(j);
    }

    void read_queue(const json& j) {
        scene_reader reader(s, "");
        reader.read_queue(j);
    }

    void read_widget_group(const json& j) {
        scene_reader reader(s, "");
        reader.read_widget_group(j);
    }

public:
    // File operations
    void save_scene_state_to_file(const std::string& filename) const {
        json state = export_exact_scene_state();
        std::ofstream file(filename);
        file << state.dump(4);
        file.close();
        DEBUG("Scene state saved to: " + filename);
    }
    
    bool load_scene_state_from_file(const std::string& filename) {
        try {
            std::ifstream file(filename);
            json state;
            file >> state;
            file.close();
            return import_exact_scene_state(state);
        } catch(const std::exception& e) {
            ERROR("Failed to load scene state: " + std::string(e.what()));
            return false;
        }
    }
    
    // Utility methods for state introspection
    size_t get_state_size() const {
        return export_exact_scene_state().dump().size();
    }
    
    std::vector<std::string> get_stateful_function_names() const {
        std::vector<std::string> names;
        for(const auto& [name, func] : s.functions) {
            names.push_back(name);
        }
        return names;
    }
};

#endif // __ENHANCED_SCENE_READER_HPP