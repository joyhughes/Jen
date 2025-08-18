#include "any_effect.hpp"
#include "effect.hpp"
#include "json.hpp"
#include "scene.hpp"
#include "life.hpp"


using json = nlohmann::json;

any_effect_fn::any_effect_fn() : name( "identity_effect_default" ) { 
    std::shared_ptr< eff_identity > f( new eff_identity );
    fn = std::ref( *f ); 
    fn_ptr = f; 
}


std::string any_effect_fn::get_type_name() const {
    return std::visit([](auto&& effect) -> std::string {
        using T = std::decay_t<decltype(effect)>;

        if constexpr (std::is_same_v<T, std::shared_ptr<eff_kaleidoscope_vec2f>>) { return "eff_kaleidoscope_vec2f"; }
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_radial_vec2f>>) return "eff_radial_vec2f";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_cartesian_vec2f>>) return "eff_cartesian_vec2f";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_composite>>) return "eff_composite";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_chooser>>) return "eff_chooser";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_identity>>) return "eff_identity";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_fill_warp_int>>) return "eff_fill_warp_int";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_feedback_ucolor>>) return "eff_feedback_ucolor";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_hsv_to_rgb_ucolor>>) return "eff_hsv_to_rgb_ucolor";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_rgb_to_hsv_ucolor>>) return "eff_rgb_to_hsv_ucolor";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_position_fill_vec2f>>) return "eff_position_fill_vec2f";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_theta_swirl_vec2f>>) return "eff_theta_swirl_vec2f";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_theta_rings_vec2f>>) return "eff_theta_rings_vec2f";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_theta_waves_vec2f>>) return "eff_theta_waves_vec2f";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_theta_saw_vec2f>>) return "eff_theta_saw_vec2f";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_theta_compression_waves_vec2f>>) return "eff_theta_compression_waves_vec2f";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_radial_tile_vec2f>>) return "eff_radial_tile_vec2f";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_radial_multiply_vec2f>>) return "eff_radial_multiply_vec2f";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_theta_rotate_vec2f>>) return "eff_theta_rotate_vec2f";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_vector_warp_ucolor>>) return "eff_vector_warp_ucolor";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_fill_ucolor>>) return "eff_fill_ucolor";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_mirror_ucolor>>) return "eff_mirror_ucolor";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_turn_ucolor>>) return "eff_turn_ucolor";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_flip_ucolor>>) return "eff_flip_ucolor";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_noise_ucolor>>) return "eff_noise_ucolor";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_checkerboard_ucolor>>) return "eff_checkerboard_ucolor";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_grayscale_ucolor>>) return "eff_grayscale_ucolor";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_invert_ucolor>>) return "eff_invert_ucolor";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_rotate_components_ucolor>>) return "eff_rotate_components_ucolor";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_rotate_hue_ucolor>>) return "eff_rotate_hue_ucolor";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_bit_plane_ucolor>>) return "eff_bit_plane_ucolor";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_crop_circle_ucolor>>) return "eff_crop_circle_ucolor";
        else if constexpr (std::is_same_v<T, std::shared_ptr<eff_n>>) return "eff_n";
        else if constexpr (std::is_same_v<T, std::shared_ptr<CA_ucolor>>) return "CA_ucolor";
        else if constexpr (std::is_same_v<T, std::shared_ptr<element>>) return "element";
        else if constexpr (std::is_same_v<T, std::shared_ptr<cluster>>) return "cluster";
        else return "unknown_effect";
    }, fn_ptr);
}

void any_effect_fn::serialize_to_json(json& j) const {
   std::visit([&j](auto&& effect) {
       using T = std::decay_t<decltype(effect)>;

       // Helper function to serialize harness values with function references
       auto serialize_harness = [](const auto& harness_field, const std::string& field_name) -> nlohmann::json {
           if (!harness_field.functions.empty()) {
               // Preserve harness structure with function references
               nlohmann::json result;
               nlohmann::json functions_array = nlohmann::json::array();
               for (const auto& func_obj : harness_field.functions) {
                   functions_array.push_back(func_obj.name);
               }
               result["functions"] = functions_array;
               result["value"] = *harness_field;
               return result;
           } else {
               // Just the current value if no functions
               return *harness_field;
           }
       };

       if constexpr (std::is_same_v<T, std::shared_ptr<eff_kaleidoscope_vec2f>>) {
           if (effect) {
               j["segments"] = serialize_harness(effect->segments, "segments");
               j["levels"] = serialize_harness(effect->levels, "levels");
               j["start"] = serialize_harness(effect->start, "start");
               j["spin"] = serialize_harness(effect->spin, "spin");
               j["expand"] = serialize_harness(effect->expand, "expand");
               j["level_start"] = serialize_harness(effect->level_start, "level_start");
               j["reflect"] = serialize_harness(effect->reflect, "reflect");
               j["reflect_levels"] = serialize_harness(effect->reflect_levels, "reflect_levels");
           }
       } else if constexpr (std::is_same_v<T, std::shared_ptr<eff_composite>>) {
           if (effect) {
               // Convert effect names to strings for JSON
               nlohmann::json effect_names = nlohmann::json::array();
               for (const auto& eff : effect->effects) {
                   effect_names.push_back(eff.name);
               }
               j["effects"] = effect_names;
           }
       } else if constexpr (std::is_same_v<T, std::shared_ptr<eff_radial_multiply_vec2f>>) {
           if (effect) {
               j["segments"] = serialize_harness(effect->segments, "segments");
               j["levels"] = serialize_harness(effect->levels, "levels");
               j["spin"] = serialize_harness(effect->spin, "spin");
               j["expand"] = serialize_harness(effect->expand, "expand");
               j["reflect"] = serialize_harness(effect->reflect, "reflect");
               j["reflect_levels"] = serialize_harness(effect->reflect_levels, "reflect_levels");
           }
       } else if constexpr (std::is_same_v<T, std::shared_ptr<eff_theta_rotate_vec2f>>) {
           if (effect) {
                j["angle"] = serialize_harness(effect->angle, "angle");
           }
       } else if constexpr (std::is_same_v<T, std::shared_ptr<eff_radial_tile_vec2f>>) {
           if (effect) {
               j["segments"] = serialize_harness(effect->segments, "segments");
               j["levels"] = serialize_harness(effect->levels, "levels");
               j["offset_x"] = serialize_harness(effect->offset_x, "offset_x");
               j["offset_y"] = serialize_harness(effect->offset_y, "offset_y");
               j["spin"] = serialize_harness(effect->spin, "spin");
               j["expand"] = serialize_harness(effect->expand, "expand");
               j["zoom_x"] = serialize_harness(effect->zoom_x, "zoom_x");
               j["zoom_y"] = serialize_harness(effect->zoom_y, "zoom_y");
               j["reflect_x"] = serialize_harness(effect->reflect_x, "reflect_x");
               j["reflect_y"] = serialize_harness(effect->reflect_y, "reflect_y");
           }
       } else if constexpr (std::is_same_v<T, std::shared_ptr<eff_theta_swirl_vec2f>>) {
           if (effect) {
               j["amount"] = serialize_harness(effect->amount, "amount");
           }
       } else if constexpr (std::is_same_v<T, std::shared_ptr<eff_theta_rings_vec2f>>) {
           if (effect) {
               j["n"] = serialize_harness(effect->n, "n");
               j["swirl"] = serialize_harness(effect->swirl, "swirl");
               j["alternate"] = serialize_harness(effect->alternate, "alternate");
           }
       } else if constexpr (std::is_same_v<T, std::shared_ptr<eff_theta_waves_vec2f>>) {
            if (effect) {
                j["freq"] = serialize_harness(effect->freq, "freq");
                j["amp"] = serialize_harness(effect->amp, "amp");
                j["phase"] = serialize_harness(effect->phase, "phase");
                j["const_amp"] = serialize_harness(effect->const_amp, "const_amp");
            }
       } else if constexpr (std::is_same_v<T, std::shared_ptr<eff_theta_saw_vec2f>>) {
           if (effect) {
               j["freq"] = serialize_harness(effect->freq, "freq");
               j["amp"] = serialize_harness(effect->amp, "amp");
               j["phase"] = serialize_harness(effect->phase, "phase");
               j["const_amp"] = serialize_harness(effect->const_amp, "const_amp");
           }
       } else if constexpr (std::is_same_v<T, std::shared_ptr<eff_theta_compression_waves_vec2f>>) {
           if (effect) {
               j["freq"] = serialize_harness(effect->freq, "freq");
               j["amp"] = serialize_harness(effect->amp, "amp");
               j["phase"] = serialize_harness(effect->phase, "phase");
               j["const_amp"] = serialize_harness(effect->const_amp, "const_amp");
           }
       } else if constexpr (std::is_same_v<T, std::shared_ptr<eff_chooser>>) {
           if (effect) {
               // Convert effect names to strings for JSON
               nlohmann::json effect_names = nlohmann::json::array();
               for (const auto& eff : effect->effects) {
                   effect_names.push_back(eff.name);
               }
               j["effects"] = effect_names;
               j["choice"] = serialize_harness(effect->choice, "choice");
           }
       } else if constexpr (std::is_same_v<T, std::shared_ptr<eff_vector_warp_ucolor>>) {
          if (effect) {
              j["vf_name"] = serialize_harness(effect->vf_name, "vf_name");
              j["step"] = serialize_harness(effect->step, "step");
              j["smooth"] = serialize_harness(effect->smooth, "smooth");
              j["relative"] = serialize_harness(effect->relative, "relative");
              j["extend"] = serialize_harness(effect->extend, "extend");
          }
       } else if constexpr (std::is_same_v<T, std::shared_ptr<eff_fill_ucolor>>) {
           if (effect) {
               j["fill_color"] = serialize_harness(effect->fill_color, "fill_color");
               j["bounded"] = effect->bounded;
               // Serialize bounding box with harness structure
               if (!effect->bounds.functions.empty()) {
                   nlohmann::json bounds_json;
                   nlohmann::json functions_array = nlohmann::json::array();
                   for (const auto& func_obj : effect->bounds.functions) {
                       functions_array.push_back(func_obj.name);
                   }
                   bounds_json["functions"] = functions_array;
                   bounds_json["value"] = {
                       {"b1", {"x", (*effect->bounds).b1.x, "y", (*effect->bounds).b1.y}},
                       {"b2", {"x", (*effect->bounds).b2.x, "y", (*effect->bounds).b2.y}}
                   };
                   j["bounds"] = bounds_json;
               } else {
                   // Serialize bounding box as individual components
                   const auto& bounds_val = *effect->bounds;
                   j["bounds"] = {
                       {"b1", {"x", bounds_val.b1.x, "y", bounds_val.b1.y}},
                       {"b2", {"x", bounds_val.b2.x, "y", bounds_val.b2.y}}
                   };
               }
           }
       } else if constexpr (std::is_same_v<T, std::shared_ptr<eff_mirror_ucolor>>) {
            if (effect) {
                j["reflect_x"] = effect->reflect_x;
                j["reflect_y"] = effect->reflect_y;
                j["top_to_bottom"] = effect->top_to_bottom;
                j["left_to_right"] = effect->left_to_right;
                j["center"] = serialize_harness(effect->center, "center"); // Only center is a harness type
                j["extend"] = effect->extend;
            }
       } else if constexpr (std::is_same_v<T, std::shared_ptr<eff_turn_ucolor>>) {
           if (effect) {
               j["direction"] = effect->direction; // direction is a direct enum field
           }
       } else if constexpr (std::is_same_v<T, std::shared_ptr<eff_flip_ucolor>>) {
           if (effect) {
               j["flip_x"] = effect->flip_x; // flip_x and flip_y are direct bool fields
               j["flip_y"] = effect->flip_y;
           }
       } else if constexpr (std::is_same_v<T, std::shared_ptr<eff_noise_ucolor>>) {
           if (effect) {
               j["a"] = serialize_harness(effect->a, "a");
               j["bounded"] = effect->bounded;
               // Serialize bounding box with harness structure
               if (!effect->bounds.functions.empty()) {
                   nlohmann::json bounds_json;
                   nlohmann::json functions_array = nlohmann::json::array();
                   for (const auto& func_obj : effect->bounds.functions) {
                       functions_array.push_back(func_obj.name);
                   }
                   bounds_json["functions"] = functions_array;
                   bounds_json["value"] = {
                       {"b1", {"x", (*effect->bounds).b1.x, "y", (*effect->bounds).b1.y}},
                       {"b2", {"x", (*effect->bounds).b2.x, "y", (*effect->bounds).b2.y}}
                   };
                   j["bounds"] = bounds_json;
               } else {
                   // Serialize bounding box as individual components
                   const auto& bounds_val = *effect->bounds;
                   j["bounds"] = {
                       {"b1", {"x", bounds_val.b1.x, "y", bounds_val.b1.y}},
                       {"b2", {"x", bounds_val.b2.x, "y", bounds_val.b2.y}}
                   };
               }
           }
       } else if constexpr (std::is_same_v<T, std::shared_ptr<eff_checkerboard_ucolor>>) {
           if (effect) {
               j["box_size"] = serialize_harness(effect->box_size, "box_size");
               j["c1"] = serialize_harness(effect->c1, "c1");
               j["c2"] = serialize_harness(effect->c2, "c2");
           }
       } else if constexpr (std::is_same_v<T, std::shared_ptr<eff_rotate_components_ucolor>>) {
           if (effect) {
               j["r"] = serialize_harness(effect->r, "r");
           }
       } else if constexpr (std::is_same_v<T, std::shared_ptr<eff_rotate_hue_ucolor>>) {
            if (effect) {
                j["offset"] = serialize_harness(effect->offset, "offset");
            }
        } else if constexpr (std::is_same_v<T, std::shared_ptr<eff_bit_plane_ucolor>>) {
            if (effect) {
                j["bit_mask"] = serialize_harness(effect->bit_mask, "bit_mask");
            }
       } else if constexpr (std::is_same_v<T, std::shared_ptr<eff_crop_circle_ucolor>>) {
           if (effect) {
               j["background"] = serialize_harness(effect->background, "background");
               j["ramp_width"] = serialize_harness(effect->ramp_width, "ramp_width");
           }
       } else if constexpr (std::is_same_v<T, std::shared_ptr<eff_n>>) {
           if (effect) {
               j["n"] = serialize_harness(effect->n, "n"); // Preserve harness structure
               if (effect->eff.name != "identity_effect_default") {
                   j["eff"] = effect->eff.name;
               }
           }
       } else if constexpr (std::is_same_v<T, std::shared_ptr<eff_fill_warp_int>>) {
           if (effect) {
               j["vf_name"] = serialize_harness(effect->vf_name, "vf_name");
               j["relative"] = serialize_harness(effect->relative, "relative");
               j["extend"] = serialize_harness(effect->extend, "extend");
           }
       } else if constexpr (std::is_same_v<T, std::shared_ptr<eff_feedback_ucolor>>) {
           if (effect) {
               j["wf_name"] = serialize_harness(effect->wf_name, "wf_name");
           }
       }

   }, fn_ptr);
}
