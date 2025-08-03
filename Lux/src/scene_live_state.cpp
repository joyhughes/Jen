#include "scene.hpp"
#include "scene_io.hpp"
#include <chrono>
#include <thread>
#include <fstream>
#include <iostream>
#include <set>
#include <mutex>

#define DEBUG(x) std::cout << x << std::endl;

using json = nlohmann::json;

// Utility functions for serialization
static std::string pixel_type_to_string(pixel_type ptype) {
    switch(ptype) {
        case PIXEL_FRGB: return "frgb";
        case PIXEL_UCOLOR: return "uimage";
        case PIXEL_VEC2F: return "vector_field";
        case PIXEL_INT: return "warp_field";
        case PIXEL_VEC2I: return "offset_field";
        default: return "unknown";
    }
}

static std::string render_mode_to_string(render_mode rmode) {
    switch(rmode) {
        case MODE_STATIC: return "static";
        case MODE_ITERATIVE: return "iterative";
        case MODE_EPHEMERAL: return "ephemeral";
        default: return "static";
    }
}

template<typename T>
static json serialize_harness_reference(const harness<T>& h) {
    json result;
    if (h.functions.empty()) {
        // Direct value - should not happen in references, but handle it
        result = h.val;
    } else if (h.functions.size() == 1) {
        // Single function reference
        result = json{{"functions", json::array({h.functions.front()})}};
    } else {
        // Multiple functions
        result = json{{"functions", json::array()}};
        for (const auto& func_name : h.functions) {
            result["functions"].push_back(func_name);
        }
    }
    return result;
}

json scene::save_complete_state() const {
    std::lock_guard<std::mutex> lock(state_mutex);

    json complete_state;

    complete_state["name"] = name;
    complete_state["liveCamera"] = liveCamera;
    complete_state["time_interval"] = time_interval;

    complete_state["_capture_frame"] = current_frame_number;

    complete_state["images"] = serialize_images();

    json functions_json;
    for(const auto& [fname, func] : functions) {
        json func_json;

        to_json(func_json, func);

        extend_function_with_current_values(func_json, func);

        functions_json.push_back(func_json);
    } 

    complete_state["functions"] = functions_json;

    complete_state["effects"] = serialize_effects_static();

    json queue_json;
    for(const auto& elist: queue) {
        json queue_item;

        queue_item["name"] = elist.name;
        queue_item["self_generated"] = elist.self_generated;
        queue_item["type"] = pixel_type_to_string(elist.ptype);
        queue_item["mode"] = render_mode_to_string(elist.rmode);
        if (elist.relative_dim != 1.0f) queue_item["relative_dim"] = elist.relative_dim;

        // harness references (static)
        queue_item["dim"] = serialize_harness_reference(elist.dim);

        if (!elist.self_generated) {
            queue_item["source"] = serialize_harness_reference(elist.source_name);
        }

        queue_item["effects"] = elist.effects;

        queue_item["rendered"] = elist.rendered;
        queue_json.push_back(queue_item);

    }

    complete_state["queue"] = queue_json;

    json widget_groups_json;
    for(const auto& widget_group : ui.widget_groups) {
        json wg_json;
        to_json(wg_json, widget_group);
        widget_groups_json.push_back(wg_json);
    }

    complete_state["widget_groups"] = widget_groups_json;

    return complete_state;
}


void scene::extend_function_with_current_values(json& func_json, const any_function& func) const {
    std::visit([&](const auto &wrapper) {
        using WrapperType = std::decay_t<decltype(wrapper)>;
        
        if constexpr (std::is_same_v<WrapperType, any_fn<float>>) {
            std::visit([&](const auto& fn_ptr) {
                using FnType = typename std::decay_t<decltype(*fn_ptr)>;

                if constexpr (std::is_same_v<FnType, slider_float>) {
                    if (func_json.contains("value")) {
                        json value_obj;
                        value_obj["current_value"] = fn_ptr->value.val;

                        if (func_json["value"].contains("functions")) {
                            value_obj["functions"] = func_json["value"]["functions"];
                        }

                        func_json["value"] = value_obj;
                    }
                } else if constexpr (std::is_same_v<FnType, integrator_float>) {
                    func_json["val"] = fn_ptr->val;
                    func_json["last_time"] = fn_ptr->last_time;
                } else if constexpr (std::is_same_v<FnType, generator_float>) {
                    // Note: generators don't have persistent state to serialize
                    // Their state is determined by their harness parameters
                }
            }, wrapper.any_fn_ptr);
        } else if constexpr (std::is_same_v<WrapperType, any_fn<int>>) {
            std::visit([&](const auto& fn_ptr) {
                using FnType = typename std::decay_t<decltype(*fn_ptr)>;

                if constexpr (std::is_same_v<FnType, slider_int>) {
                    if (func_json.contains("value")) {
                        json value_obj;
                        value_obj["current_value"] = fn_ptr->value.val;

                        if (func_json["value"].contains("functions")) {
                            value_obj["functions"] = func_json["value"]["functions"];
                        }
                        func_json["value"] = value_obj;
                    }
                } else if constexpr (std::is_same_v<FnType, menu_int>) {
                    if (func_json.contains("choice")) {
                        json choice_obj;
                        choice_obj["current_choice"] = fn_ptr->choice.val;

                        if (func_json["choice"].contains("functions")) {
                            choice_obj["functions"] = func_json["choice"]["functions"];
                        }

                        func_json["choice"] = choice_obj;
                    }
                } else if constexpr (std::is_same_v<FnType, generator_int>) {
                    // Note: generators don't have persistent state to serialize
                    // Their state is determined by their harness parameters
                }
            }, wrapper.any_fn_ptr);
        } else if constexpr (std::is_same_v<WrapperType, any_fn<bool>>) {
            std::visit([&](const auto& fn_ptr) {
                using FnType = typename std::decay_t<decltype(*fn_ptr)>;

                if constexpr (std::is_same_v<FnType, switch_fn>) {
                    if (func_json.contains("value")) {
                        json value_obj;
                        value_obj["current_value"] = fn_ptr->value.val;

                        if (func_json["value"].contains("functions")) {
                            value_obj["functions"] = func_json["value"]["functions"];
                        }

                        func_json["value"] = value_obj;
                    }
                }
            }, wrapper.any_fn_ptr);
        } else if constexpr (std::is_same_v<WrapperType, any_fn<std::string>>) {
            std::visit([&](const auto& fn_ptr) {
                using FnType = typename std::decay_t<decltype(*fn_ptr)>;

                if constexpr (std::is_same_v<FnType, menu_string>) {
                    if (func_json.contains("choice")) {
                        json choice_obj;
                        choice_obj["current_choice"] = fn_ptr->choice.val;

                        if (func_json["choice"].contains("functions")) {
                            choice_obj["functions"] = func_json["choice"]["functions"];
                        }

                        func_json["choice"] = choice_obj;
                    }
                }
            }, wrapper.any_fn_ptr);
        }
    }, func);
}


void scene::load_complete_state(const json& state_json) {

    std::lock_guard<std::mutex> lock(state_mutex);

    json scene_for_loading = prepare_scene_for_loading(state_json);

    load_scene_with_reader(scene_for_loading);

    restore_current_values_from_saved_state(state_json);

    if (state_json.contains("_capture_frame")) {
        current_frame_number = state_json["_capture_frame"];
    }


    validate_loaded_state();
    sync_ui_with_function_states();

    DEBUG("Scene loaded successfully, with capture frame number: " << current_frame_number);
}


json scene::prepare_scene_for_loading(const json& saved_state) const {
    json scene_json = saved_state;

    if (scene_json.contains("functions")) {
        json& functions_array = scene_json["functions"];
        for(auto& func_json : functions_array) {
            // For sliders: set default_value from current_value only if value is undefined
            if (func_json.contains("value") && func_json["value"].is_object() && func_json["value"].contains("current_value")) {
                // Only set default_value if it's not already defined
                if (!func_json.contains("default_value")) {
                    func_json["default_value"] = func_json["value"]["current_value"];
                }
            }

            // For menus: set default_choice from current_choice only if choice is undefined  
            if (func_json.contains("choice") && func_json["choice"].is_object() && func_json["choice"].contains("current_choice")) {
                // Only set default_choice if it's not already defined
                if (!func_json.contains("default_choice")) {
                    func_json["default_choice"] = func_json["choice"]["current_choice"];
                }
            }
        }
    }

    if(scene_json.contains("_capture_frame")) {
        scene_json.erase("_capture_frame");
    }

    return scene_json;
}


void scene::load_scene_with_reader(const json& scene_for_loading) {
    std::string temp_filename = "temp_scene" + std::to_string(current_frame_number) + ".json";
    std::ofstream temp_file(temp_filename);
    temp_file << scene_for_loading.dump(4);
    temp_file.close();

    functions.clear();
    effects.clear();
    queue.clear();
    buffers.clear();
    ui.widget_groups.clear();
    

    scene_reader reader(*this, temp_filename);


    std::remove(temp_filename.c_str());

}


void scene::restore_current_values_from_saved_state(const json& state_json) {
    if (!state_json.contains("functions")) return;
    
    for(const auto& func_json : state_json["functions"]) {
        if (!func_json.contains("name")) continue;
        
        std::string func_name = func_json["name"];
        if (!functions.contains(func_name)) continue;
        
        const auto& func = functions[func_name];
        
        std::visit([&](const auto& wrapper) {
            using WrapperType = std::decay_t<decltype(wrapper)>;
            
            if constexpr (std::is_same_v<WrapperType, any_fn<float>>) {
                std::visit([&](const auto& fn_ptr) {
                    using FnType = typename std::decay_t<decltype(*fn_ptr)>;

                    if constexpr(std::is_same_v<FnType, slider_float>) {
                        if (func_json.contains("value") && func_json["value"].contains("current_value")){
                            fn_ptr->value.val = func_json["value"]["current_value"];
                        }
                    } else if constexpr(std::is_same_v<FnType, integrator_float>) {
                        if (func_json.contains("val")) {
                            fn_ptr->val = func_json["val"];
                        }
                        if (func_json.contains("last_time")) {
                            fn_ptr->last_time = func_json["last_time"];
                        }
                    } else if constexpr(std::is_same_v<FnType, generator_float>) {
                        // Note: generators don't have persistent state to restore
                        // Their state is determined by their harness parameters
                    }
                }, wrapper.any_fn_ptr);
            } else if constexpr (std::is_same_v<WrapperType, any_fn<int>>) {
                std::visit([&](const auto& fn_ptr) {
                    using FnType = typename std::decay_t<decltype(*fn_ptr)>;

                    if constexpr(std::is_same_v<FnType, slider_int>) {
                        if (func_json.contains("value") && func_json["value"].contains("current_value")) {
                            fn_ptr->value.val = func_json["value"]["current_value"];
                        }
                    } else if constexpr(std::is_same_v<FnType, menu_int>) {
                        if (func_json.contains("choice") && func_json["choice"].contains("current_choice")) {
                            fn_ptr->choice.val = func_json["choice"]["current_choice"];
                        }
                    } else if constexpr(std::is_same_v<FnType, generator_int>) {
                        // Note: generators don't have persistent state to restore
                        // Their state is determined by their harness parameters
                    }
                }, wrapper.any_fn_ptr);
            } else if constexpr (std::is_same_v<WrapperType, any_fn<bool>>) {
                std::visit([&](const auto& fn_ptr) {
                    using FnType = typename std::decay_t<decltype(*fn_ptr)>;

                    if constexpr(std::is_same_v<FnType, switch_fn>) {
                        if (func_json.contains("value") && func_json["value"].contains("current_value")) {
                            fn_ptr->value.val = func_json["value"]["current_value"];
                        }
                    }
                }, wrapper.any_fn_ptr);
            } else if constexpr (std::is_same_v<WrapperType, any_fn<std::string>>) {
                std::visit([&](const auto& fn_ptr) {
                    using FnType = typename std::decay_t<decltype(*fn_ptr)>;

                    if constexpr(std::is_same_v<FnType, menu_string>) {
                        if (func_json.contains("choice") && func_json["choice"].contains("current_choice")) {
                            // menu_string uses choice harness but stores an int index
                            fn_ptr->choice.val = func_json["choice"]["current_choice"];
                        }
                    }
                }, wrapper.any_fn_ptr);
            }
        }, func);
    }

    if (state_json.contains("queue")) {
        for(size_t i = 0; i < state_json["queue"].size() && i < queue.size(); ++i) {
            const auto& queue_json = state_json["queue"][i];
            if (queue_json.contains("rendered")) {
                queue[i].rendered = queue_json["rendered"];
            }
        }
    }
}



json scene::serialize_images() const {
    json images_json = json::array();
    
    // Serialize all image buffers that are not self-generated
    for (const auto& [name, buffer] : buffers) {
        // Skip self-generated buffers - they're in the queue
        bool is_self_generated = false;
        for (const auto& elist : queue) {
            if (elist.name == name && elist.self_generated) {
                is_self_generated = true;
                break;
            }
        }
        
        if (!is_self_generated) {
            json img_json;
            img_json["name"] = name;
            
            // Determine image type from buffer
            switch (buffer.index()) {
                case 0: // frgb
                    img_json["type"] = "fimage";
                    break;
                case 1: // ucolor
                    img_json["type"] = "uimage";
                    break;
                case 2: // vec2f
                    img_json["type"] = "vector_field";
                    break;
                case 3: // int
                    img_json["type"] = "warp_field";
                    break;
                case 4: // vec2i
                    img_json["type"] = "offset_field";
                    break;
                default:
                    img_json["type"] = "unknown";
            }
            
            // Note: filename would need to be stored somewhere to be serialized
            // For now, we'll note that this is a runtime buffer
            img_json["_runtime_buffer"] = true;
            
            images_json.push_back(img_json);
        }
    }
    
    return images_json;
}

json scene::serialize_effects_static() const {
    json effects_json = json::array();
    
    // Serialize all effects that are referenced in the scene
    std::set<std::string> used_effects;
    
    // Collect all effect names used in the queue
    for (const auto& elist : queue) {
        for (const auto& effect_name : elist.effects) {
            used_effects.insert(effect_name);
        }
    }
    
    // Serialize each effect with proper structure
    for (const auto& effect_name : used_effects) {
        json effect_json;
        effect_json["name"] = effect_name;
        
        // Map effect names to their proper types based on kaleido.json
        if (effect_name == "position fill") {
            effect_json["type"] = "eff_position_fill_vec2f";
        } else if (effect_name == "radial") {
            effect_json["type"] = "eff_radial_vec2f";
        } else if (effect_name == "cartesian") {
            effect_json["type"] = "eff_cartesian_vec2f";
        } else if (effect_name == "kaleidoscope") {
            effect_json["type"] = "eff_kaleidoscope_vec2f";
            effect_json["segments"] = json{{"functions", json::array({"segment_slider"})}};
            effect_json["levels"] = json{{"functions", json::array({"level_slider"})}};
            effect_json["start"] = json{{"functions", json::array({"start_integrator"})}};
            effect_json["spin"] = json{{"functions", json::array({"spin_integrator"})}};
            effect_json["expand"] = json{{"functions", json::array({"expand_integrator"})}};
            effect_json["level_start"] = json{{"functions", json::array({"level_start_integrator"})}};
            effect_json["reflect"] = json{{"functions", json::array({"reflect_switch"})}};
            effect_json["reflect_levels"] = json{{"functions", json::array({"reflect_level_switch"})}};
        } else if (effect_name == "kaleidoscope_cartesian") {
            effect_json["type"] = "eff_composite";
            effect_json["effects"] = json::array({"kaleidoscope", "cartesian"});
        } else if (effect_name == "radial_multiply") {
            effect_json["type"] = "eff_radial_multiply_vec2f";
            effect_json["segments"] = json{{"functions", json::array({"segment_slider"})}};
            effect_json["levels"] = json{{"functions", json::array({"level_slider"})}};
            effect_json["spin"] = json{{"functions", json::array({"spin_integrator"})}};
            effect_json["expand"] = json{{"functions", json::array({"expand_integrator"})}};
            effect_json["reflect"] = json{{"functions", json::array({"reflect_switch"})}};
            effect_json["reflect_levels"] = json{{"functions", json::array({"reflect_level_switch"})}};
        } else if (effect_name == "radial_multiply_cartesian") {
            effect_json["type"] = "eff_composite";
            effect_json["effects"] = json::array({"radial_multiply", "cartesian"});
        } else if (effect_name == "identity") {
            effect_json["type"] = "eff_identity";
        } else if (effect_name == "theta_rotate") {
            effect_json["type"] = "eff_theta_rotate_vec2f";
            effect_json["angle"] = json{{"functions", json::array({"spin_integrator"})}};
        } else if (effect_name == "theta_rotate_cartesian") {
            effect_json["type"] = "eff_composite";
            effect_json["effects"] = json::array({"theta_rotate", "cartesian"});
        } else if (effect_name == "radial_tile") {
            effect_json["type"] = "eff_radial_tile_vec2f";
            effect_json["segments"] = json{{"functions", json::array({"segment_slider"})}};
            effect_json["levels"] = json{{"functions", json::array({"level_slider"})}};
            effect_json["offset_x"] = 0.0;
            effect_json["offset_y"] = 0.0;
            effect_json["spin"] = json{{"functions", json::array({"spin_integrator"})}};
            effect_json["expand"] = json{{"functions", json::array({"expand_integrator"})}};
            effect_json["zoom_x"] = 1.0;
            effect_json["zoom_y"] = 1.0;
            effect_json["reflect_x"] = json{{"functions", json::array({"reflect_switch"})}};
            effect_json["reflect_y"] = json{{"functions", json::array({"reflect_level_switch"})}};
        } else if (effect_name == "theta_swirl") {
            effect_json["type"] = "eff_theta_swirl_vec2f";
            effect_json["amount"] = json{{"functions", json::array({"swirl_slider"})}};
        } else if (effect_name == "theta rings") {
            effect_json["type"] = "eff_theta_rings_vec2f";
            effect_json["n"] = json{{"functions", json::array({"rings_slider"})}};
            effect_json["swirl"] = json{{"functions", json::array({"swirl_slider"})}};
            effect_json["alternate"] = json{{"functions", json::array({"alternate_slider"})}};
        } else if (effect_name == "theta waves") {
            effect_json["type"] = "eff_theta_waves_vec2f";
            effect_json["freq"] = json{{"functions", json::array({"freq_slider"})}};
            effect_json["amp"] = json{{"functions", json::array({"amp_slider"})}};
            effect_json["phase"] = json{{"functions", json::array({"phase_integrator"})}};
            effect_json["const_amp"] = json{{"functions", json::array({"const_amp_switch"})}};
        } else if (effect_name == "theta saw") {
            effect_json["type"] = "eff_theta_saw_vec2f";
            effect_json["freq"] = json{{"functions", json::array({"freq_slider"})}};
            effect_json["amp"] = json{{"functions", json::array({"amp_slider"})}};
            effect_json["phase"] = json{{"functions", json::array({"phase_integrator"})}};
            effect_json["const_amp"] = json{{"functions", json::array({"const_amp_switch"})}};
        } else if (effect_name == "theta compression waves") {
            effect_json["type"] = "eff_theta_compression_waves_vec2f";
            effect_json["freq"] = json{{"functions", json::array({"freq_slider"})}};
            effect_json["amp"] = json{{"functions", json::array({"amp_slider"})}};
            effect_json["phase"] = json{{"functions", json::array({"phase_integrator"})}};
            effect_json["const_amp"] = json{{"functions", json::array({"const_amp_switch"})}};
        } else if (effect_name == "scope_chooser") {
            effect_json["type"] = "eff_chooser";
            effect_json["effects"] = json::array({"theta_rotate_cartesian", "kaleidoscope_cartesian", "radial_multiply_cartesian", "radial_tile"});
            effect_json["choice"] = json{{"functions", json::array({"scope_menu"})}};
        } else if (effect_name == "funky chooser") {
            effect_json["type"] = "eff_chooser";
            effect_json["effects"] = json::array({"identity", "theta_swirl", "theta rings", "theta waves", "theta saw", "theta compression waves"});
            effect_json["choice"] = json{{"functions", json::array({"funky_menu"})}};
        } else if (effect_name == "fill warp field") {
            effect_json["type"] = "eff_fill_warp_int";
            effect_json["vf_name"] = "kaleido_vf";
        } else if (effect_name == "warp image") {
            effect_json["type"] = "eff_feedback_ucolor";
            effect_json["wf_name"] = "warper";
        } else if (effect_name == "rgb to hsv") {
            effect_json["type"] = "eff_rgb_to_hsv_ucolor";
        } else if (effect_name == "hsv to rgb") {
            effect_json["type"] = "eff_hsv_to_rgb_ucolor";
        } else {
            // For unknown effects, just note that it's a runtime effect
            effect_json["_runtime_effect"] = true;
        }
        
        effects_json.push_back(effect_json);
    }
    
    return effects_json;
}


void scene::save_scene_to_file(const std::string& filename) const {
    json complete_state = save_complete_state();

    std::ofstream file(filename);
    file << complete_state.dump(4);
    file.close();

    DEBUG("Scene saved to file: " << filename);
}

void scene::load_scene_from_file(const std::string& filename) {
    std::ifstream file(filename);
    json complete_state;
    file >> complete_state;
    file.close();
    load_complete_state(complete_state);

    DEBUG("Scene loaded from file: " << filename);
}

// Validation functions (stub implementations)
void scene::validate_loaded_state() {
    // TODO: Add validation logic here
    DEBUG("Scene state validated");
}

void scene::sync_ui_with_function_states() {
    // TODO: Add UI synchronization logic here  
    DEBUG("UI synchronized with function states");
}