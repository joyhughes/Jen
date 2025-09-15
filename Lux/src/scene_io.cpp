#include "scene.hpp"
#include "scene_io.hpp"
#include "life.hpp"
#include "fimage.hpp"
#include "uimage.hpp"
#include "vector_field.hpp"
#include "UI.hpp"
#include "json.hpp"
#include <fstream>
#include <sstream>
#include <type_traits>

#define DEBUG( msg ) { std::string debug_msg = msg; std::cout << debug_msg << std::endl; }
#define ERROR( msg ) throw std::runtime_error( msg );

using json = nlohmann::json;
using string = std::string;


scene_reader::scene_reader(scene &s_init, std::string (filename)) : s(s_init) {
    // read a JSON file
    DEBUG("scene_reader constructor");
#ifdef __EMSCRIPTEN__
    DEBUG( "emscripten defined" )
#endif // __EMSCRIPTEN__
    std::ifstream in_stream(filename);
    json j;
    DEBUG("input stream opened " + filename)
    try {
        j = json::parse(in_stream);
    } catch (json::parse_error &ex) {
        DEBUG("JSON parse error at byte " + std::to_string( ex.byte ))
        ERROR("JSON parse error at byte " + std::to_string( ex.byte ))
        exit(0);
    }
    DEBUG("scene file parsed into json object")
    // For filename constructor, detect whether this has runtime state or is a default config
    bool has_runtime = has_runtime_state(j);
    initialize_from_json(j, has_runtime);
}


scene_reader::scene_reader(scene &s_init, const nlohmann::json& scene_json, bool load_runtime_state)
    : s(s_init) {
    DEBUG("scene_reader constructor from JSON object");
    initialize_from_json(scene_json, load_runtime_state);
}


vec2f scene_reader::read_vec2f(const json &j) { return vec2f({j[0], j[1]}); }
vec2i scene_reader::read_vec2i(const json &j) { return vec2i({j[0], j[1]}); }
interval_float scene_reader::read_interval_float(const json &j) { return interval_float(j[0], j[1]); }
interval_int scene_reader::read_interval_int(const json &j) { return interval_int(j[0], j[1]); }
frgb scene_reader::read_frgb(const json &j) { return frgb(j[0], j[1], j[2]); }
bb2f scene_reader::read_bb2f(const json &j) { return bb2f(read_vec2f(j[0]), read_vec2f(j[1])); }
bb2i scene_reader::read_bb2i(const json &j) { return bb2i(read_vec2i(j[0]), read_vec2i(j[1])); }

std::string scene_reader::read_string(const json &j) {
    std::string s;
    j.get_to(s);
    return s;
}

// ucolor represented as hexidecimal string
ucolor scene_reader::read_ucolor(const json &j) {
    std::string color;
    ucolor u;

    j.get_to(color);
    std::istringstream(color) >> std::hex >> u;
    return u;
}

funk_factor scene_reader::read_funk_factor(const json &j) {
    std::string s;
    unsigned long long ull;
    j.get_to(s);
    std::istringstream(s) >> std::hex >> ull;
    return ull;
}

rotation_direction scene_reader::read_rotation_direction(const json &j) {
    std::string s;
    rotation_direction r;

    j.get_to(s);
    if (s == "counterclockwise") r = COUNTERCLOCKWISE;
    else if (s == "clockwise") r = CLOCKWISE;
    else if (s == "random") r = RANDOM;
    else if (s == "lava lamp") r = LAVA_LAMP;
    else
        ERROR("Invalid rotation_direction string: " + s)
    return r;
}

direction4 scene_reader::read_direction4(const json &j) {
    std::string s;
    direction4 d;

    j.get_to(s);
    if (s == "up") d = direction4::D4_UP;
    else if (s == "right") d = direction4::D4_RIGHT;
    else if (s == "down") d = direction4::D4_DOWN;
    else if (s == "left") d = direction4::D4_LEFT;
    else
        ERROR("Invalid direction4 string: " + s)
    return d;
}

direction4_diagonal scene_reader::read_direction4_diagonal(const json &j) {
    std::string s;
    direction4_diagonal d;

    j.get_to(s);
    if (s == "up_right") d = direction4_diagonal::D4D_UPRIGHT;
    else if (s == "down_right") d = direction4_diagonal::D4D_DOWNRIGHT;
    else if (s == "down_left") d = direction4_diagonal::D4D_DOWNLEFT;
    else if (s == "up_left") d = direction4_diagonal::D4D_UPLEFT;
    else
        ERROR("Invalid direction4_diagonal string: " + s)
    return d;
}

direction8 scene_reader::read_direction8(const json &j) {
    std::string s;
    direction8 d;

    j.get_to(s);
    if (s == "up") d = direction8::D8_UP;
    else if (s == "up_right") d = direction8::D8_UPRIGHT;
    else if (s == "right") d = direction8::D8_RIGHT;
    else if (s == "down_right") d = direction8::D8_DOWNRIGHT;
    else if (s == "down") d = direction8::D8_DOWN;
    else if (s == "down_left") d = direction8::D8_DOWNLEFT;
    else if (s == "left") d = direction8::D8_LEFT;
    else if (s == "up_left") d = direction8::D8_UPLEFT;
    else
        ERROR("Invalid direction8 string: " + s)
    return d;
}

box_blur_type scene_reader::read_box_blur_type(const json &j) {
    std::string s;
    box_blur_type b;

    j.get_to(s);
    if (s == "orthogonal") b = box_blur_type::BB_ORTHOGONAL;
    else if (s == "diagonal") b = box_blur_type::BB_DIAGONAL;
    else if (s == "all") b = box_blur_type::BB_ALL;
    else if (s == "custom") b = box_blur_type::BB_CUSTOM;
    else
        ERROR("Invalid box_blur_type string: " + s)
    return b;
}

pixel_type scene_reader::read_pixel_type(const json &j) {
    std::string s;
    pixel_type p;

    j.get_to(s);
    if (s == "fimage" || s == "frgb") p = pixel_type::PIXEL_FRGB;
    else if (s == "uimage" || s == "ucolor") p = pixel_type::PIXEL_UCOLOR;
    else if (s == "vector_field" || s == "vec2f") p = pixel_type::PIXEL_VEC2F;
    else if (s == "offset_field" || s == "vec2i") p = pixel_type::PIXEL_VEC2I;
    else if (s == "warp_field" || s == "int") p = pixel_type::PIXEL_INT;
    else
        ERROR("Invalid pixel_type string: " + s)
    return p;
}

image_extend scene_reader::read_image_extend(const json &j) {
    std::string s;
    image_extend e;

    j.get_to(s);
    if (s == "single") e = image_extend::SAMP_SINGLE;
    else if (s == "repeat") e = image_extend::SAMP_REPEAT;
    else if (s == "reflect") e = image_extend::SAMP_REFLECT;
    else
        ERROR("Invalid image_extend string: " + s)
    return e;
}

render_mode scene_reader::read_render_mode(const json &j) {
    std::string s;
    render_mode r;

    j.get_to(s);
    if (s == "static") r = render_mode::MODE_STATIC;
    else if (s == "iterative") r = render_mode::MODE_ITERATIVE;
    else if (s == "ephemeral") r = render_mode::MODE_EPHEMERAL;
    else
        ERROR("Invalid render_mode string: " + s)
    return r;
}

CA_hood scene_reader::read_hood(const json &j) {
    std::string s;
    CA_hood h;

    j.get_to(s);
    if (s == "Moore") h = HOOD_MOORE;
    else if (s == "Margolus") h = HOOD_MARGOLUS;
    else if (s == "hourglass") h = HOOD_HOUR;
    else if (s == "hourglass_reverse") h = HOOD_HOUR_REV;
    else if (s == "bowtie") h = HOOD_BOW;
    else if (s == "bowtie_reverse") h = HOOD_BOW_REV;
    else if (s == "square") h = HOOD_SQUARE;
    else if (s == "square_reverse") h = HOOD_SQUARE_REV;
    else if (s == "random") h = HOOD_RANDOM;

    else
        ERROR("Invalid CA_hood string: " + s)
    return h;
}

probability_distribution scene_reader::read_probability_distribution(const json &j) {
    probability_distribution pd;
    std::string s;
    j.get_to(s);
    if (s == "uniform") pd = PROB_UNIFORM;
    else if (s == "normal") pd = PROB_NORMAL;
    else if (s == "poisson") pd = PROB_POISSON;
    else if (s == "exponential") pd = PROB_EXPONENTIAL;
    else if (s == "binomial") pd = PROB_BINOMIAL;
    else if (s == "geometric") pd = PROB_GEOMETRIC;
    else if (s == "hypergeometric") pd = PROB_HYPERGEOMETRIC;
    else if (s == "negative_binomial") pd = PROB_NEGATIVE_BINOMIAL;
    else if (s == "log_normal") pd = PROB_LOG_NORMAL;
    else if (s == "weibull") pd = PROB_WEIBULL;
    else if (s == "cauchy") pd = PROB_CAUCHY;
    else
        ERROR("Invalid probability_distribution string: " + s)
    return pd;
}

menu_type scene_reader::read_menu_type(const json &j) {
    std::string s;
    menu_type t;

    j.get_to(s);
    if (s == "pull_down") t = MENU_PULL_DOWN;
    else if (s == "radio") t = MENU_RADIO;
    else if (s == "image") t = MENU_IMAGE;
    else
        ERROR("Invalid menu_type string: " + s)
    return t;
}

switch_type scene_reader::read_switch_type(const json &j) {
    std::string s;
    switch_type t;

    j.get_to(s);
    if (s == "switch") t = SWITCH_SWITCH;
    else if (s == "checkbox") t = SWITCH_CHECKBOX;
    else
        ERROR("Invalid switch_type string: " + s)
    return t;
}

void scene_reader::read_image(const json &j) {
    std::string type, name, filename;

    if (j.contains("filename")) j["filename"].get_to(filename);
    else
        ERROR("scene_reader::read_image error - image filename missing\n")

    if (j.contains("name")) j["name"].get_to(name);
    else name = filename;

    if (j.contains("type")) j["type"].get_to(type);
    else
        ERROR("scene_reader::read_image error - image type missing\n")

    // future: add binary file format for all image types

    DEBUG("scene_reader::read_image() - filename " + filename + "name " + name + " type " + type)
    
    try {
        if (type == "fimage") {
            fbuf_ptr img(new buffer_pair<frgb>(filename));
            s.buffers[name] = img;
        }

        if (type == "uimage") {
            ubuf_ptr img(new buffer_pair<ucolor>(filename));
            s.buffers[name] = img;
            //std :: cout << "uimage pointer " << img << std::endl;
            //s.buffers[ name ] = std::make_shared< buffer_pair< ucolor > >( filename );
        }
    } catch (const std::exception& e) {
        DEBUG("Warning: Could not load image " + filename + " - " + e.what() + " (skipping)")
        // Continue without this image - effects that reference it will use fallbacks
    }
    // future: binary image format, which will support fimage, uimage, and additionally vector_field
    DEBUG("scene_reader::read_image - finished")
}

void scene_reader::read_element(const json &j) {
    std::string name, img, mask, mask_mode;
    // If no image, element not rendered, serves as placeholder

    if (j.contains("name")) j["name"].get_to(name);
    else
        ERROR("Element name missing\n")
    s.elements[name] = std::make_shared<element>();
    element &elem = *(s.elements[name]);

    if (j.contains("smooth")) j["smooth"].get_to(elem.smooth);
    if (j.contains("position")) elem.position = read_vec2f(j["position"]);
    if (j.contains("scale")) j["scale"].get_to(elem.scale);
    if (j.contains("rotation")) j["rotation"].get_to(elem.rotation);
    if (j.contains("orientation")) j["orientation"].get_to(elem.orientation);
    if (j.contains("orientation_lock")) j["orientation_lock"].get_to(elem.orientation_lock);
    // Set initial derivative - may need to be modified by initializers in next_element
    if (j.contains("derivative")) elem.derivative = read_vec2f(j["derivative"]);
    if (j.contains("derivative_lock")) j["derivative_lock"].get_to(elem.derivative_lock);
    if (j.contains("mask_mode")) j["mask_mode"].get_to(mask_mode);

    if (mask_mode == "no_effect") elem.mmode = MASK_NOEFFECT;
    if (mask_mode == "trim") elem.mmode = MASK_TRIM;
    if (mask_mode == "opacity") elem.mmode = MASK_OPACITY;
    if (mask_mode == "blend") elem.mmode = MASK_BLEND;

    // image, mask, tint
    if (j.contains("image")) {
        j["image"].(img);
        elem_img_bufs[name] = img;
    }

    if (j.contains("mask")) {
        j["mask"].get_to(mask);
        elem_mask_bufs[name] = mask;
    }

    if (j.contains("tint")) {
        auto k = j["tint"];
        if (k.contains("frgb")) elem.tint = read_frgb(k["frgb"]);
        if (k.contains("ucolor")) elem.tint = read_ucolor(k["ucolor"]);
        if (k.contains("vec2f")) elem.tint = read_vec2f(k["vec2f"]);
    }
    std::cout << "read element " << name << " smooth " << elem.smooth << std::endl;
}

// forward references not implemented
template<class T>
void scene_reader::read_harness(const json &j, harness<T> &h) {
    if (j.is_object()) {
        if (j.contains("value")) {
            T old_val = h.val;
            read(h.val, j["value"]);
            
            // For saved scenes, we need to also assign this runtime value
            // to the function's value field so the frontend can access it
            if (is_saved_scene) {
                // Handle different types specially for debug output
                if constexpr (std::is_same_v<T, interval_float> || std::is_same_v<T, interval_int>) {
                    std::cout << "DEBUG: read_harness - assigning runtime value [" << h.val.min << ", " << h.val.max << "] to harness" << std::endl;
                } else if constexpr (std::is_same_v<T, vec2f> || std::is_same_v<T, vec2i>) {
                    std::cout << "DEBUG: read_harness - assigning runtime value [" << h.val.x << ", " << h.val.y << "] to harness" << std::endl;
                } else if constexpr (std::is_same_v<T, frgb>) {
                    std::cout << "DEBUG: read_harness - assigning runtime value (frgb) to harness" << std::endl;
                } else if constexpr (std::is_same_v<T, bb2f> || std::is_same_v<T, bb2i>) {
                    std::cout << "DEBUG: read_harness - assigning runtime value (bounding_box) to harness" << std::endl;
                } else if constexpr (std::is_same_v<T, string>) {
                    std::cout << "DEBUG: read_harness - assigning runtime value '" << h.val << "' to harness" << std::endl;
                } else if constexpr (std::is_same_v<T, bool>) {
                    std::cout << "DEBUG: read_harness - assigning runtime value " << (h.val ? "true" : "false") << " to harness" << std::endl;
                } else if constexpr (std::is_enum_v<T>) {
                    std::cout << "DEBUG: read_harness - assigning runtime value (enum) to harness" << std::endl;
                } else if constexpr (std::is_arithmetic_v<T>) {
                    std::cout << "DEBUG: read_harness - assigning runtime value " << h.val << " to harness" << std::endl;
                } else {
                    std::cout << "DEBUG: read_harness - assigning runtime value (complex type) to harness" << std::endl;
                }
            }
        }
        // For now, skip function restoration during initial loading to avoid crashes
        // Functions will be restored later after all functions are loaded
        if (j.contains("functions")) {
            std::cout << "DEBUG: Deferring harness function restoration for " << j["functions"].size() << " functions" << std::endl;
        }
    } else read(h.val, j);
}

void scene_reader::add_default_functions() {
    // time function
    std::shared_ptr<time_fn> timer(new time_fn);
    s.functions["timer"] = any_fn<float>(timer, std::ref(*timer), "timer");

    // UI functions
    std::shared_ptr<mouse_pos_fn> mouse_position(new mouse_pos_fn);
    s.functions["mouse_position"] = any_fn<vec2f>(mouse_position, std::ref(*mouse_position), "mouse_position");
    std::shared_ptr<mouse_pix_fn> mouse_pixel(new mouse_pix_fn);
    s.functions["mouse_pixel"] = any_fn<vec2i>(mouse_pixel, std::ref(*mouse_pixel), "mouse_pixel");

    // UI conditions
    std::shared_ptr<mousedown_condition> mouse_down(new mousedown_condition);
    s.functions["mouse_down"] = any_condition_fn(mouse_down, std::ref(*mouse_down), "mouse_down");
    std::shared_ptr<mouseover_condition> mouse_over(new mouseover_condition);
    s.functions["mouse_over"] = any_condition_fn(mouse_over, std::ref(*mouse_over), "mouse_over");
    std::shared_ptr<mouseclick_condition> mouse_click(new mouseclick_condition);
    s.functions["mouse_click"] = any_condition_fn(mouse_click, std::ref(*mouse_click), "mouse_click");

    // Cluster conditions
    std::shared_ptr<initial_element_condition> initial_element(new initial_element_condition);
    s.functions["initial_element"] = any_condition_fn(initial_element, std::ref(*initial_element), "initial_element");
    std::shared_ptr<following_element_condition> following_element(new following_element_condition);
    s.functions["following_element"] = any_condition_fn(following_element, std::ref(*following_element),
                                                        "following_element");
    std::shared_ptr<top_level_condition> top_level(new top_level_condition);
    s.functions["top_level"] = any_condition_fn(top_level, std::ref(*top_level), "top_level");
    std::shared_ptr<lower_level_condition> lower_level(new lower_level_condition);
    s.functions["lower_level"] = any_condition_fn(lower_level, std::ref(*lower_level), "lower_level");
}

void scene_reader::initialize_from_json(const nlohmann::json& j, bool load_runtime_state) {
    // Simple runtime state detection - if load_runtime_state is true, assume it's a saved scene
    is_saved_scene = load_runtime_state;
    
    if (is_saved_scene) {
        std::cout << "DEBUG: Loading saved scene with runtime state preservation" << std::endl;
    } else {
        std::cout << "DEBUG: Loading default scene configuration (no runtime state)" << std::endl;
    }

    // Store the JSON for deferred harness restoration
    if (is_saved_scene) {
        deferred_harness_json = j;
    }

    // Basic scene fields
    if (j.contains("name")) j["name"].get_to(s.name);
    else s.name = "Unnamed";

    if (j.contains("time_interval")) j["time_interval"].get_to(s.time_interval);
    if (j.contains("liveCamera")) j["liveCamera"].get_to(s.liveCamera);
    else s.liveCamera = false;

    // Load all components in order
    if (j.contains("images")) {
        for (auto &jimg: j["images"]) read_image(jimg);
    }
    DEBUG("Images loaded");

    if (j.contains("elements")) {
        for (auto &jelem: j["elements"]) read_element(jelem);
    }
    DEBUG("Elements loaded");

    add_default_functions();
    DEBUG("Default functions added");

    if (j.contains("functions")) {
        for (auto &jfunc: j["functions"]) {
            read_function(jfunc);
        }
    }
    DEBUG("Functions loaded");

    if (j.contains("clusters")) {
        for (auto &jclust: j["clusters"]) read_cluster(jclust);
    }
    DEBUG("Clusters loaded");

    if (j.contains("effects")) {
        for (auto &jeff: j["effects"]) read_effect(jeff);
    }
    DEBUG("Effects loaded");

    if (j.contains("queue")) {
        for (auto &q: j["queue"]) read_queue(q);
    }

    if (j.contains("widget groups")) {
        for (auto &wg: j["widget groups"]) read_widget_group(wg);
    }
    DEBUG("Widget groups loaded");

    // Finalize setup (existing logic)
    for (auto &e: elem_img_bufs) { s.elements[e.first]->img = s.buffers[e.second]; }
    for (auto &m: elem_mask_bufs) { s.elements[m.first]->mask = s.buffers[m.second]; }
    for (auto &c: cluster_elements) { s.clusters[c.first]->root_elem = *s.elements[c.second]; }
    for (auto &t: CA_targets) {
        std::get<std::shared_ptr<CA<ucolor>>>(s.effects[t.first].fn_ptr)->target = s.buffers[t.second];
    }

    // Now restore harness functions after all functions are loaded
    if (is_saved_scene) {
        restore_harness_functions();
    }

    DEBUG("scene_reader initialization finished");
    

}

bool scene_reader::has_runtime_state(const json &scene_json) {
    // Check for SAVED scene indicators - only true runtime state, not default config function wiring
    if (scene_json.contains("functions")) {
        for (const auto& func : scene_json["functions"]) {
            // Check for integrator functions with non-default runtime values (strong indicator)
            if (func.contains("type") && func["type"] == "integrator_float" && func.contains("val")) {
                double val = func["val"];
                if (std::abs(val) > 0.001) { // Non-zero integrator value indicates runtime state
                    std::cout << "DEBUG: Found integrator with runtime state '" << func["name"] << "' val=" << val << std::endl;
                    std::cout << "DEBUG: Scene has integrator runtime values, treating as saved scene" << std::endl;
                    return true;
                }
            }
            
            // Check for SAVED harness values - look for actual runtime values stored alongside functions
            if (func.contains("value") && func["value"].is_object()) {
                const auto& value_obj = func["value"];
                if (value_obj.contains("functions") && value_obj.contains("value")) {
                    // This is a harness object with both function references AND a saved runtime value
                    std::cout << "DEBUG: Found saved harness with both functions and runtime value in '" << func["name"] << "'" << std::endl;
                    std::cout << "DEBUG: Scene has saved harness runtime values, treating as saved scene" << std::endl;
                    return true;
                }
            }
            
            // Check for SAVED choice harness with runtime values (menus)
            if (func.contains("choice") && func["choice"].is_object()) {
                const auto& choice_obj = func["choice"];
                if (choice_obj.contains("functions") && choice_obj.contains("value")) {
                    // This is a choice harness with both function references AND a saved runtime value
                    std::cout << "DEBUG: Found saved menu choice with both functions and runtime value in '" << func["name"] << "'" << std::endl;
                    std::cout << "DEBUG: Scene has saved menu runtime values, treating as saved scene" << std::endl;
                    return true;
                }
            }
        }
    }
    
    std::cout << "DEBUG: Scene has no saved runtime state, treating as default configuration" << std::endl;
    std::cout << "DEBUG: Function references without saved values are normal for default scenes" << std::endl;
    return false;
}


void scene_reader::read_function(const json &j) {
    std::string name, type, fn_name;

    if (j.contains("name")) j["name"].get_to(name);
    else
        ERROR("Function name missing\n")
    if (j.contains("type")) j["type"].get_to(type);
    else
        ERROR("Function type missing\n");

    DEBUG("scene_reader::read_function - name: " + name + " type: " + type)

    // special case for conditionals
    if (type == "filter") {
        std::shared_ptr<filter> fn(new filter);
        any_gen_fn gen_func(fn, std::ref(*fn), name);
        if (j.contains("conditions"))
            for (auto &c: j["conditions"])
                fn->add_condition(
                    std::get<any_condition_fn>(s.functions[c]));
        if (j.contains("functions"))
            for (auto &f: j["functions"])
                fn->add_function(
                    std::get<any_gen_fn>(s.functions[f]));
        s.functions[name] = gen_func;
    }

    // Add filter function handler in to_json

    // example of expanded macro sequence
    /* if( type == "orientation_gen_fn" ) {
        auto fn = orientation_gen_fn();
        any_gen_fn func( std::make_shared< orientation_gen_fn >( fn ), std::ref( fn ), name );
        if( j.contains( "orientation" ) ) read_any_harness( j[ "orientation" ], fn. orientation );
        s.functions[ name ] = func;
    } */

#define FN( _T_, _U_ ) if( type == #_T_ )     { std::shared_ptr< _T_ > fn( new _T_ ); any_fn< _U_ >    func( fn, std::ref( *fn ), name );
#define END_FN  s.functions[ name ] = func; }
#define GEN_FN( _T_ )  if( type == #_T_ )     { std::shared_ptr< _T_ > fn( new _T_ ); any_gen_fn       func( fn, std::ref( *fn ), name );
#define END_GEN_FN  s.functions[ name ] = func; }
#define COND_FN( _T_ ) if( type == #_T_ )     { std::shared_ptr< _T_ > fn( new _T_ ); any_condition_fn func( fn, std::ref( *fn ), name );
#define END_COND_FN  s.functions[ name ] = func; }

#define HARNESS( _T_ ) if( j.contains( #_T_ ) ) read_harness( j[ #_T_ ], fn-> _T_ );
#define READ( _T_ )    if( j.contains( #_T_ ) ) read( fn-> _T_, j[ #_T_ ] );
#define PARAM( _T_ )   if( j.contains( "fn" ) ) { j[ "fn" ].get_to( fn_name ); fn->fn = std::get< any_fn< _T_ > >( s.functions[ fn_name ] ); }

    // harness bool functions
    FN(switch_fn, bool)
        READ(tool)
        READ(label)
        READ(description)
        READ(default_value)
        
        // Set default value first
        fn->value = fn->default_value;
        
        // For saved scenes, extract runtime value from either harness or direct value
        if (is_saved_scene && j.contains("value")) {
            if (j["value"].is_object()) {
                // Harness object format: {"functions": [...], "value": true}
                if (j["value"].contains("value")) {
                    bool runtime_value;
                    read(runtime_value, j["value"]["value"]);
                    fn->value = runtime_value;
                    std::cout << "DEBUG: switch_fn '" << name << "' - assigned harness runtime value: " << runtime_value << std::endl;
                    
                    // Defer harness function restoration to avoid crashes during initial loading
                    if (j["value"].contains("functions")) {
                        std::cout << "DEBUG: switch_fn '" << name << "' - deferring restoration of " << j["value"]["functions"].size() << " harness functions" << std::endl;
                    }
                }
            } else if (j["value"].is_boolean()) {
                // Direct value format: "value": true
                bool runtime_value;
                read(runtime_value, j["value"]);
                fn->value = runtime_value;
                std::cout << "DEBUG: switch_fn '" << name << "' - assigned direct runtime value: " << runtime_value << std::endl;
            }
        }
    END_FN

    // harness float functions
    FN(adder_float, float)
        HARNESS(r)
    END_FN
    FN(tweaker_float, float)
        HARNESS(p)
        HARNESS(amount)
        HARNESS(enabled)
    END_FN
    FN(generator_float, float)
        READ(distribution)
        HARNESS(p)
        HARNESS(a)
        HARNESS(b)
        HARNESS(enabled)
    END_FN
    FN(log_fn, float)
        HARNESS(scale)
        HARNESS(shift)
    END_FN
    FN(time_fn, float)
    END_FN
    FN(ratio_float, float)
        HARNESS(r)
    END_FN
    FN(integrator_float, float)
        HARNESS(delta)
        HARNESS(scale)
        READ(val)
    END_FN
    FN(wiggle, float)
        HARNESS(wavelength)
        HARNESS(amplitude)
        HARNESS(phase)
        HARNESS(wiggliness)
    END_FN
    FN(slider_float, float)
        READ(label)
        READ(description)
        READ(min)
        READ(max)
        READ(default_value)
        READ(step)
        
        // Set default value first
        fn->value = fn->default_value;
        
        // For saved scenes, extract runtime value from either harness or direct value
        if (is_saved_scene && j.contains("value")) {
            if (j["value"].is_object()) {
                // Harness object format: {"functions": [...], "value": 20}
                if (j["value"].contains("value")) {
                    float runtime_value;
                    read(runtime_value, j["value"]["value"]);
                    fn->value = runtime_value;
                    std::cout << "DEBUG: slider_float '" << name << "' - assigned harness runtime value: " << runtime_value << std::endl;
                    
                    // Defer harness function restoration to avoid crashes during initial loading
                    if (j["value"].contains("functions")) {
                        std::cout << "DEBUG: slider_float '" << name << "' - deferring restoration of " << j["value"]["functions"].size() << " harness functions" << std::endl;
                    }
                }
            } else if (j["value"].is_number()) {
                // Direct value format: "value": 20
                float runtime_value;
                read(runtime_value, j["value"]);
                fn->value = runtime_value;
                std::cout << "DEBUG: slider_float '" << name << "' - assigned direct runtime value: " << runtime_value << std::endl;
            }
        }
    END_FN
    FN(range_slider_float, interval_float)
        READ(label)
        READ(description)
        READ(min)
        READ(max)
        READ(default_value)
        READ(step)
        
        // Set default value first
        fn->value = fn->default_value;
        
        // For saved scenes, extract runtime value from harness JSON
        if (is_saved_scene && j.contains("value") && j["value"].is_object()) {
            if (j["value"].contains("value")) {
                interval_float runtime_value;
                read(runtime_value, j["value"]["value"]);
                fn->value = runtime_value;
                std::cout << "DEBUG: range_slider_float '" << name << "' - assigned runtime value: [" << runtime_value.min << ", " << runtime_value.max << "]" << std::endl;
                
                // Note: range_slider_float uses interval<float>, not harness<interval<float>>, so no functions to restore
            }
        }
    END_FN

    // audio function - combines multiple channels and effects
    FN(audio_adder_fn, float)
        HARNESS(volume_channel)
        HARNESS(volume_weight)
        HARNESS(volume_sensitivity)
        HARNESS(bass_channel)
        HARNESS(bass_weight)
        HARNESS(bass_sensitivity)
        HARNESS(mid_channel)
        HARNESS(mid_weight)
        HARNESS(mid_sensitivity)
        HARNESS(high_channel)
        HARNESS(high_weight)
        HARNESS(high_sensitivity)
        HARNESS(offset)
        HARNESS(global_sensitivity)
    END_FN

    // harness int functions
    FN(adder_int, int)
        HARNESS(r)
    END_FN
    FN(generator_int, int)
        READ(distribution)
        HARNESS(p)
        HARNESS(a)
        HARNESS(b)
        HARNESS(enabled)
    END_FN
    FN(slider_int, int)
        READ(label)
        READ(description)
        READ(min)
        READ(max)
        READ(default_value)
        READ(step)
        
        // Set default value first
        fn->value = fn->default_value;
        
        // For saved scenes, extract runtime value from either harness or direct value
        if (is_saved_scene && j.contains("value")) {
            if (j["value"].is_object()) {
                // Harness object format: {"functions": [...], "value": 20}
                if (j["value"].contains("value")) {
                    int runtime_value;
                    read(runtime_value, j["value"]["value"]);
                    fn->value = runtime_value;
                    std::cout << "DEBUG: slider_int '" << name << "' - assigned harness runtime value: " << runtime_value << std::endl;
                    
                    // Defer harness function restoration to avoid crashes during initial loading
                    if (j["value"].contains("functions")) {
                        std::cout << "DEBUG: slider_int '" << name << "' - deferring restoration of " << j["value"]["functions"].size() << " harness functions" << std::endl;
                    }
                }
            } else if (j["value"].is_number()) {
                // Direct value format: "value": 20
                int runtime_value;
                read(runtime_value, j["value"]);
                fn->value = runtime_value;
                std::cout << "DEBUG: slider_int '" << name << "' - assigned direct runtime value: " << runtime_value << std::endl;
            }
        }
    END_FN
    FN(range_slider_int, interval_int)
        READ(label)
        READ(description)
        READ(min)
        READ(max)
        READ(default_value)
        READ(step)
        
        // Set default value first
        fn->value = fn->default_value;
        
        // For saved scenes, extract runtime value from harness JSON
        if (is_saved_scene && j.contains("value") && j["value"].is_object()) {
            if (j["value"].contains("value")) {
                interval_int runtime_value;
                read(runtime_value, j["value"]["value"]);
                fn->value = runtime_value;
                std::cout << "DEBUG: range_slider_int '" << name << "' - assigned runtime value: [" << runtime_value.min << ", " << runtime_value.max << "]" << std::endl;
                
                // Note: range_slider_int uses interval<int>, not harness<interval<int>>, so no functions to restore
            }
        }
    END_FN

    // special case for menu
    FN(menu_int, int)
        READ(label)
        READ(description)
        READ(default_choice)
        READ(tool)
        READ(affects_widget_groups)
        READ(rerender)
        
        // Set default choice first
        fn->choice = fn->default_choice;
        
        // For saved scenes, extract runtime choice from either harness or direct value
        if (is_saved_scene && j.contains("choice")) {
            if (j["choice"].is_object()) {
                // Harness object format: {"functions": [...], "value": 1}
                if (j["choice"].contains("value")) {
                    int runtime_choice;
                    read(runtime_choice, j["choice"]["value"]);
                    fn->choice = runtime_choice;
                    std::cout << "DEBUG: menu_int '" << name << "' - assigned harness runtime choice: " << runtime_choice << std::endl;
                    
                    // Defer harness function restoration to avoid crashes during initial loading
                    if (j["choice"].contains("functions")) {
                        std::cout << "DEBUG: menu_int '" << name << "' - deferring restoration of " << j["choice"]["functions"].size() << " harness functions" << std::endl;
                    }
                }
            } else if (j["choice"].is_number()) {
                // Direct value format: "choice": 1
                int runtime_choice;
                read(runtime_choice, j["choice"]);
                fn->choice = runtime_choice;
                std::cout << "DEBUG: menu_int '" << name << "' - assigned direct runtime choice: " << runtime_choice << std::endl;
            }
        }
        
        if (j.contains("items")) for (std::string item: j["items"]) fn->add_item(item);
    END_FN

    FN(menu_string, string)
        READ(label)
        READ(description)
        READ(default_choice)
        READ(tool)
        READ(affects_widget_groups)
        READ(rerender)
        
        // Set default choice first
        fn->choice = fn->default_choice;
        
        // For saved scenes, extract runtime choice from either harness or direct value
        if (is_saved_scene && j.contains("choice")) {
            if (j["choice"].is_object()) {
                // Harness object format: {"functions": [...], "value": 1}
                if (j["choice"].contains("value")) {
                    int runtime_choice;
                    read(runtime_choice, j["choice"]["value"]);
                    fn->choice = runtime_choice;
                    std::cout << "DEBUG: menu_string '" << name << "' - assigned harness runtime choice: " << runtime_choice << std::endl;
                    
                    // Defer harness function restoration to avoid crashes during initial loading
                    if (j["choice"].contains("functions")) {
                        std::cout << "DEBUG: menu_string '" << name << "' - deferring restoration of " << j["choice"]["functions"].size() << " harness functions" << std::endl;
                    }
                }
            } else if (j["choice"].is_number()) {
                // Direct value format: "choice": 1
                int runtime_choice;
                read(runtime_choice, j["choice"]);
                fn->choice = runtime_choice;
                std::cout << "DEBUG: menu_string '" << name << "' - assigned direct runtime choice: " << runtime_choice << std::endl;
            }
        }
        
        if (j.contains("items")) for (std::string item: j["items"]) fn->add_item(item);
    END_FN

    // harness vec2f functions
    FN(adder_vec2f, vec2f)
        HARNESS(r)
    END_FN
    FN(ratio_vec2f, vec2f)
        HARNESS(r)
    END_FN
    FN(mouse_pos_fn, vec2f)
    END_FN


    // harness vec2i functions
    FN(adder_vec2i, vec2i)
        HARNESS(r)
    END_FN
    FN(buffer_dim_fn, vec2i)
        HARNESS(buf_name)
    END_FN
    FN(mouse_pix_fn, vec2i)
    END_FN

    // harness frgb functions
    FN(adder_frgb, frgb)
        HARNESS(r)
    END_FN

    // harness ucolor functions
    FN(adder_ucolor, ucolor)
        HARNESS(r)
    END_FN

    // pickers
    FN(ucolor_picker, ucolor)
        READ(label)
        READ(description)
        READ(default_value)
        fn->value = fn->default_value;
    END_FN
    FN(funk_factor_picker, funk_factor)
        READ(label)
        READ(description)
        READ(default_value)
        fn->value = fn->default_value;
    END_FN
    FN(direction_picker_4, direction4)
        READ(label)
        READ(description)
        READ(default_value)
        fn->value = fn->default_value;
    END_FN
    FN(direction_picker_4_diagonal, direction4_diagonal)
        READ(label)
        READ(description)
        READ(default_value)
        fn->value = fn->default_value;
    END_FN
    FN(direction_picker_8, direction8)
        READ(label)
        READ(description)
        READ(default_value)
        fn->value = fn->default_value;
    END_FN
    FN(multi_direction8_picker, int)
        READ(label)
        READ(description)
        READ(default_value)
        fn->value = fn->default_value;
    END_FN
    FN(box_blur_picker, box_blur_type)
        READ(label)
        READ(description)
        READ(default_value)
        fn->value = fn->default_value;
    END_FN

    // special case for custom_blur_picker
    FN(custom_blur_picker, int)
        READ(label)
        READ(description)
        // create list of pairs of multi_direction8_pickers
        if (j.contains("pickers")) {
            for (auto &p: j["pickers"]) {
                fn->add_pickers(p[0], p[1]);
            }
        } else {
            fn->add_pickers(17, 17); // default orthogonal box blur
            fn->add_pickers(68, 68);
        }
    END_FN

    // harness bool functions
    FN(random_toggle, bool)
        HARNESS(enabled)
        HARNESS(p)
    END_FN
    FN(random_fn, bool)
        HARNESS(p)
    END_FN
    FN(random_sticky_fn, bool)
        HARNESS(p_start)
        HARNESS(p_change_true)
        HARNESS(p_change_false)
    END_FN
    // equality functions
    FN(equal_float_fn, bool)
        HARNESS(a)
        HARNESS(b)
    END_FN
    FN(equal_vec2f_fn, bool)
        HARNESS(a)
        HARNESS(b)
    END_FN
    FN(equal_int_fn, bool)
        HARNESS(a)
        HARNESS(b)
    END_FN
    FN(equal_vec2i_fn, bool)
        HARNESS(a)
        HARNESS(b)
    END_FN
    FN(equal_frgb_fn, bool)
        HARNESS(a)
        HARNESS(b)
    END_FN
    FN(equal_ucolor_fn, bool)
        HARNESS(a)
        HARNESS(b)
    END_FN
    FN(equal_string_fn, bool)
        HARNESS(a)
        HARNESS(b)
    END_FN
    FN(equal_bool_fn, bool)
        HARNESS(a)
        HARNESS(b)
    END_FN
    FN(equal_direction4_fn, bool)
        HARNESS(a)
        HARNESS(b)
    END_FN
    FN(equal_direction4_diagonal_fn, bool)
        HARNESS(a)
        HARNESS(b)
    END_FN
    FN(equal_direction8_fn, bool)
        HARNESS(a)
        HARNESS(b)
    END_FN


    // ui functions
    FN(switch_fn, bool)
        READ(tool)
        READ(label)
        READ(description)
        HARNESS(value)
        READ(default_value)
        READ(affects_widget_groups)
        fn->value = fn->default_value;
    END_FN
    // special case for widget switch
    FN(widget_switch_fn, bool)
        READ(switcher)
        READ(widget)
        READ(label)
        READ(description)
    END_FN

    // parameter functions
    FN(index_param_float, float)
        PARAM(float)
    END_FN
    FN(scale_param_float, float)
        PARAM(float)
    END_FN
    FN(time_param_float, float)
        PARAM(float)
    END_FN

    // Add missing parameter function handlers in to_json

    // single field modifiers
    GEN_FN(orientation_gen_fn)
        HARNESS(orientation)
    END_FN
    GEN_FN(scale_gen_fn)
        HARNESS(scale)
    END_FN
    GEN_FN(rotation_gen_fn)
        HARNESS(r)
    END_FN
    GEN_FN(position_gen_fn)
        HARNESS(position)
    END_FN

    // generalized functions (alphabetical order)
    GEN_FN(advect_element)
        HARNESS(flow)
        HARNESS(step)
        READ(proportional)
        READ(orientation_sensitive)
    END_FN
    GEN_FN(angle_branch)
        READ(interval)
        READ(offset)
        READ(mirror_offset)
        HARNESS(size_prop)
        HARNESS(branch_ang)
        HARNESS(branch_dist)
    END_FN
    GEN_FN(curly)
        HARNESS(curliness)
    END_FN
    // position_list should go here - figure out how to work the vector of positions

    // condition functions
    COND_FN(switch_condition)
        READ(tool)
        READ(label)
        READ(description)
        HARNESS(value)
        READ(default_value)
        READ(affects_widget_groups)
        fn->value = fn->default_value;
    END_FN
    COND_FN(random_condition)
        HARNESS(p)
    END_FN
    COND_FN(random_sticky_condition)
        HARNESS(p_start)
        HARNESS(p_change_true)
        HARNESS(p_change_false)
    END_FN
    // equality conditions
    COND_FN(equal_float_condition)
        HARNESS(a)
        HARNESS(b)
    END_FN
    COND_FN(equal_vec2f_condition)
        HARNESS(a)
        HARNESS(b)
    END_FN
    COND_FN(equal_int_condition)
        HARNESS(a)
        HARNESS(b)
    END_FN
    COND_FN(equal_vec2i_condition)
        HARNESS(a)
        HARNESS(b)
    END_FN
    COND_FN(equal_frgb_condition)
        HARNESS(a)
        HARNESS(b)
    END_FN
    COND_FN(equal_ucolor_condition)
        HARNESS(a)
        HARNESS(b)
    END_FN
    COND_FN(equal_string_condition)
        HARNESS(a)
        HARNESS(b)
    END_FN
    COND_FN(equal_bool_condition)
        HARNESS(a)
        HARNESS(b)
    END_FN
    COND_FN(equal_direction4_condition)
        HARNESS(a)
        HARNESS(b)
    END_FN
    COND_FN(equal_direction4_diagonal_condition)
        HARNESS(a)
        HARNESS(b)
    END_FN
    COND_FN(equal_direction8_condition)
        HARNESS(a)
        HARNESS(b)
    END_FN
}

void scene_reader::read_cluster(const json &j) {
    std::string name;
    element root_elem; // initial element in cluster
    std::string root_elem_name;
    int max_depth; // prevent infinite recursion
    float min_scale; // approximately one pixel
    bb2f bounds; // Optionally, cluster will stop generating if it goes out of bounds
    bool tlc;

    // Required fields
    if (j.contains("name")) j["name"].get_to(name);
    else
        ERROR("Cluster name missing\n")
    // Check for unique name. Future - make sure duplicate clusters refer to the same cluster
    if (s.clusters.contains(name))
        ERROR("Cluster name collision\n")
    if (j.contains("element")) j["element"].get_to(root_elem_name);
    else
        ERROR("Cluster root_elem missing\n")

    // create cluster object
    s.next_elements[name] = std::make_shared<next_element>();
    s.clusters[name] = std::make_shared<cluster>(*s.elements[root_elem_name], *s.next_elements[name]);
    cluster &clust = *(s.clusters[name]); // reference to cluster object
    cluster_elements[name] = root_elem_name;

    // optional fields
    if (j.contains("max_depth")) read_any_harness(j["max_depth"], clust.max_depth);
    if (j.contains("min_scale")) read_any_harness(j["min_scale"], clust.min_scale);
    if (j.contains("max_n")) read_any_harness(j["max_n"], clust.max_n);
    if (j.contains("functions"))
        for (std::string fname: j["functions"])
            clust.next_elem.add_function(
                std::get<any_gen_fn>(s.functions[fname]));
    if (j.contains("conditions"))
        for (std::string fname: j["conditions"])
            clust.next_elem.add_condition(
                std::get<any_condition_fn>(s.functions[fname]));
}

void scene_reader::read_rule(const json &j, std::shared_ptr<CA_ucolor> &ca) {
    std::string name, type;

    // Required fields
    if (j.contains("name")) j["name"].get_to(name);
    else
        ERROR("CA rule name missing\n")
    DEBUG("Reading CA rule " + name);
    if (j.contains("type")) j["type"].get_to(type);
    else
        ERROR("CA rule type missing\n")

#define RULE( _T_ )    if( type == #_T_ ) {  std::shared_ptr< _T_ > r( new _T_ ); any_rule rule( r, std::ref( *r ), std::ref( *r ), name ); ca->rule = rule;
#define HARNESSR( _T_ ) if( j.contains( #_T_ ) ) read_any_harness( j[ #_T_ ], r-> _T_ );
#define READR( _T_ )    if( j.contains( #_T_ ) ) read( r-> _T_, j[ #_T_ ] );
#define END_RULE()     s.CA_rules[ name ] = rule; }

    RULE(rule_life_ucolor)
        READR(use_threshold)
        HARNESSR(threshold)
    END_RULE()
    RULE(rule_random_copy_ucolor)
    END_RULE()
    RULE(rule_random_mix_ucolor)
    END_RULE()
    RULE(rule_box_blur_ucolor)
        HARNESSR(max_diff)
        HARNESSR(bug_mode)
        HARNESSR(blur_method)
        HARNESSR(random_copy)
        READR(custom_picker)
    END_RULE()
    RULE(rule_diffuse_ucolor)
    END_RULE()
    RULE(rule_gravitate_ucolor)
        HARNESSR(direction)
    END_RULE()
    RULE(rule_snow_ucolor)
        HARNESSR(direction)
    END_RULE()
    RULE(rule_pixel_sort_ucolor)
        HARNESSR(direction)
        HARNESSR(max_diff)
    END_RULE()
    RULE(rule_funky_sort_ucolor)
        HARNESSR(direction)
        HARNESSR(max_diff)
        HARNESSR(dafunk_l)
        HARNESSR(dafunk_r)
        READR(hood)
    END_RULE()
    RULE(rule_diagonal_funky_sort_ucolor)
        HARNESSR(direction)
        HARNESSR(max_diff)
        HARNESSR(dafunk_d)
        READR(hood)
    END_RULE()
    DEBUG("CA rule " + name + " complete")
}

void scene_reader::read_effect(const json &j) {
    std::string name, type, buf_name;

    // Required fields
    if (j.contains("name")) j["name"].get_to(name);
    else
        ERROR("Effect name missing\n")
    DEBUG("Reading effect " + name)
    if (j.contains("type")) j["type"].get_to(type);
    else
        ERROR("Effect type missing\n")
    // Check for unique name. Future - make sure duplicate effects refer to the same effect

    if (s.effects.contains(name))
        ERROR("Effect name collision\n")

#define EFF( _T_ )     if( type == #_T_ ) {  std::shared_ptr< _T_ > e( new _T_ ); any_effect_fn eff( e, std::ref( *( e.get() ) ), name );
#define HARNESSE( _T_ ) if( j.contains( #_T_ ) ) read_any_harness( j[ #_T_ ], e-> _T_ );
#define READE( _T_ )    if( j.contains( #_T_ ) ) read( e-> _T_, j[ #_T_ ] );
#define END_EFF()      s.effects[ name ] = eff; }

    // special case for element and cluster effects
    if (type == "element") {
        if (j.contains("element_name")) {
            std::string elem_name;
            j["element_name"].get_to(elem_name);
            if (s.elements.contains(elem_name)) {
                //std::shared_ptr< element > e( new element( *s.elements[ elem_name ] ) );
                s.effects[name] = any_effect_fn(s.elements[elem_name], std::ref(*(s.elements[elem_name].get())), name);
            } else
                ERROR("element effect not found\n")
        } else
            ERROR("element effect missing\n")
    }

    if (type == "cluster") {
        if (j.contains("cluster_name")) {
            std::string clust_name;
            j["cluster_name"].get_to(clust_name);
            if (s.clusters.contains(clust_name)) {
                //std::shared_ptr< cluster > c( new cluster( *s.clusters[ clust_name ] ) );
                s.effects[name] = any_effect_fn(s.clusters[clust_name], std::ref(*(s.clusters[clust_name].get())),
                                                name);
            } else
                ERROR("cluster effect not found\n")
        } else
            ERROR("cluster effect missing\n")
    }

    // special case for CA rules
    EFF(CA_ucolor)
        if (j.contains("rule")) read_rule(j["rule"], e);
        if (j.contains("target")) {
            j["target"].get_to(buf_name);
            CA_targets[name] = buf_name;
        }
        HARNESSE(targeted)
        HARNESSE(p)
        HARNESSE(edge_block)
        HARNESSE(alpha_block)
        HARNESSE(bright_block)
        HARNESSE(bright_range)
    END_EFF()

    // special case for effects running effects
    EFF(eff_n)
        HARNESSE(n)
        if (j.contains("eff")) {
            std::string eff_name;
            j["eff"].get_to(eff_name);
            if (s.effects.contains(eff_name)) e->eff = s.effects[eff_name];
            else
                ERROR("eff_n effect not found\n")
        }
        // else identity effect - should be automatic
    END_EFF()

    EFF(eff_composite)
        if (j.contains("effects")) {
            for (std::string eff_name: j["effects"]) {
                if (s.effects.contains(eff_name)) e->add_effect(s.effects[eff_name]);
                else
                    ERROR("eff_composite effect not found\n")
            }
        }
        // else empty effect list - should be automatic
    END_EFF()

    EFF(eff_chooser)
        if (j.contains("effects")) {
            for (std::string eff_name: j["effects"]) {
                if (s.effects.contains(eff_name)) {
                    e->add_effect(s.effects[eff_name]);
                    DEBUG("chooser effect " + name + " adding effect " + eff_name)
                } else
                    ERROR("eff_chooser effect not found\n")
            }
        }
        DEBUG("chooser effect " + name + " has " + std::to_string( e->effects.size() ) + " effects")
        // else empty effect list - should be automatic
        HARNESSE(choice)
    END_EFF()

    EFF(eff_identity)
    END_EFF()
    EFF(eff_fill_frgb)
        HARNESSE(fill_color)
        READE(bounded)
        HARNESSE(bounds)
    END_EFF()
    EFF(eff_fill_ucolor)
        HARNESSE(fill_color)
        READE(bounded)
        HARNESSE(bounds)
    END_EFF()
    EFF(eff_fill_vec2i)
        HARNESSE(fill_color)
        READE(bounded)
        HARNESSE(bounds)
    END_EFF()
    EFF(eff_fill_vec2f)
        HARNESSE(fill_color)
        READE(bounded)
        HARNESSE(bounds)
    END_EFF()
    EFF(eff_fill_int)
        HARNESSE(fill_color)
        READE(bounded)
        HARNESSE(bounds)
    END_EFF()

    EFF(eff_grayscale_frgb)
    END_EFF()
    EFF(eff_grayscale_ucolor)
    END_EFF()

    EFF(eff_invert_ucolor)
    END_EFF()
    EFF(eff_invert_frgb)
    END_EFF()

    EFF(eff_rotate_components_frgb)
        HARNESSE(r)
    END_EFF()
    EFF(eff_rotate_components_ucolor)
        HARNESSE(r)
    END_EFF()

    EFF(eff_rgb_to_hsv_frgb)
    END_EFF()
    EFF(eff_rgb_to_hsv_ucolor)
    END_EFF()

    EFF(eff_hsv_to_rgb_frgb)
    END_EFF()
    EFF(eff_hsv_to_rgb_ucolor)
    END_EFF()

    EFF(eff_rotate_hue_frgb)
        HARNESSE(offset)
    END_EFF()
    EFF(eff_rotate_hue_ucolor)
        HARNESSE(offset)
    END_EFF()

    EFF(eff_bit_plane_ucolor)
        HARNESSE(bit_mask)
    END_EFF()

    EFF(eff_crop_circle_frgb)
        HARNESSE(background)
        HARNESSE(ramp_width)
    END_EFF()
    EFF(eff_crop_circle_ucolor)
        HARNESSE(background)
        HARNESSE(ramp_width)
    END_EFF()
    EFF(eff_crop_circle_vec2i)
        HARNESSE(background)
        HARNESSE(ramp_width)
    END_EFF()
    EFF(eff_crop_circle_vec2f)
        HARNESSE(background)
        HARNESSE(ramp_width)
    END_EFF()
    EFF(eff_crop_circle_int)
        HARNESSE(background)
        HARNESSE(ramp_width)
    END_EFF()

    EFF(eff_mirror_frgb)
        READE(reflect_x)
        READE(reflect_y)
        READE(top_to_bottom)
        READE(left_to_right)
        HARNESSE(center)
        READE(extend)
    END_EFF()
    EFF(eff_mirror_ucolor)
        READE(reflect_x)
        READE(reflect_y)
        READE(top_to_bottom)
        READE(left_to_right)
        HARNESSE(center)
        READE(extend)
    END_EFF()
    EFF(eff_mirror_vec2i)
        READE(reflect_x)
        READE(reflect_y)
        READE(top_to_bottom)
        READE(left_to_right)
        HARNESSE(center)
        READE(extend)
    END_EFF()
    EFF(eff_mirror_vec2f)
        READE(reflect_x)
        READE(reflect_y)
        READE(top_to_bottom)
        READE(left_to_right)
        HARNESSE(center)
        READE(extend)
    END_EFF()
    EFF(eff_mirror_int)
        READE(reflect_x)
        READE(reflect_y)
        READE(top_to_bottom)
        READE(left_to_right)
        HARNESSE(center)
        READE(extend)
    END_EFF()

    EFF(eff_turn_frgb)
        READE(direction)
    END_EFF()
    EFF(eff_turn_ucolor)
        READE(direction)
    END_EFF()
    EFF(eff_turn_vec2i)
        READE(direction)
    END_EFF()
    EFF(eff_turn_vec2f)
        READE(direction)
    END_EFF()
    EFF(eff_turn_int)
        READE(direction)
    END_EFF()

    EFF(eff_flip_frgb)
        READE(flip_x)
        READE(flip_y)
    END_EFF()
    EFF(eff_flip_ucolor)
        READE(flip_x)
        READE(flip_y)
    END_EFF()
    EFF(eff_flip_vec2i)
        READE(flip_x)
        READE(flip_y)
    END_EFF()
    EFF(eff_flip_vec2f)
        READE(flip_x)
        READE(flip_y)
    END_EFF()
    EFF(eff_flip_int)
        READE(flip_x)
        READE(flip_y)
    END_EFF()

    EFF(eff_noise_frgb)
        HARNESSE(a)
        READE(bounded)
        HARNESSE(bounds)
    END_EFF()
    EFF(eff_noise_ucolor)
        HARNESSE(a)
        READE(bounded)
        HARNESSE(bounds)
    END_EFF()
    EFF(eff_noise_vec2i)
        HARNESSE(a)
        READE(bounded)
        HARNESSE(bounds)
    END_EFF()
    EFF(eff_noise_vec2f)
        HARNESSE(a)
        READE(bounded)
        HARNESSE(bounds)
    END_EFF()
    EFF(eff_noise_int)
        HARNESSE(a)
        READE(bounded)
        HARNESSE(bounds)
    END_EFF()

    EFF(eff_checkerboard_frgb)
        HARNESSE(box_size)
        HARNESSE(c1)
        HARNESSE(c2)
    END_EFF()
    EFF(eff_checkerboard_ucolor)
        HARNESSE(box_size)
        HARNESSE(c1)
        HARNESSE(c2)
    END_EFF()
    EFF(eff_checkerboard_vec2i)
        HARNESSE(box_size)
        HARNESSE(c1)
        HARNESSE(c2)
    END_EFF()
    EFF(eff_checkerboard_vec2f)
        HARNESSE(box_size)
        HARNESSE(c1)
        HARNESSE(c2)
    END_EFF()
    EFF(eff_checkerboard_int)
        HARNESSE(box_size)
        HARNESSE(c1)
        HARNESSE(c2)
    END_EFF()

    EFF(eff_vector_warp_frgb)
        HARNESSE(vf_name)
        HARNESSE(step)
        HARNESSE(smooth)
        HARNESSE(relative)
        HARNESSE(extend)
    END_EFF()
    EFF(eff_vector_warp_ucolor)
        HARNESSE(vf_name)
        HARNESSE(step)
        HARNESSE(smooth)
        HARNESSE(relative)
        HARNESSE(extend)
    END_EFF()
    EFF(eff_vector_warp_vec2i)
        HARNESSE(vf_name)
        HARNESSE(step)
        HARNESSE(smooth)
        HARNESSE(relative)
        HARNESSE(extend)
    END_EFF()
    EFF(eff_vector_warp_vec2f)
        HARNESSE(vf_name)
        HARNESSE(step)
        HARNESSE(smooth)
        HARNESSE(relative)
        HARNESSE(extend)
    END_EFF()
    EFF(eff_vector_warp_int)
        HARNESSE(vf_name)
        HARNESSE(step)
        HARNESSE(smooth)
        HARNESSE(relative)
        HARNESSE(extend)
    END_EFF()

    EFF(eff_feedback_frgb)
        HARNESSE(wf_name)
    END_EFF()
    EFF(eff_feedback_ucolor)
        HARNESSE(wf_name)
    END_EFF()
    EFF(eff_feedback_vec2i)
        HARNESSE(wf_name)
    END_EFF()
    EFF(eff_feedback_vec2f)
        HARNESSE(wf_name)
    END_EFF()
    EFF(eff_feedback_int)
        HARNESSE(wf_name)
    END_EFF()

    // vector field effects
    EFF(eff_complement_vec2f)
    END_EFF()
    EFF(eff_radial_vec2f)
    END_EFF()
    EFF(eff_cartesian_vec2f)
    END_EFF()
    EFF(eff_rotate_vectors_vec2f)
        HARNESSE(angle)
    END_EFF()
    EFF(eff_scale_vectors_vec2f)
        HARNESSE(scale)
    END_EFF()
    EFF(eff_normalize_vec2f)
    END_EFF()
    EFF(eff_inverse_vec2f)
        HARNESSE(diameter)
        HARNESSE(soften)
    END_EFF()
    EFF(eff_inverse_square_vec2f)
        HARNESSE(diameter)
        HARNESSE(soften)
    END_EFF()
    EFF(eff_concentric_vec2f)
        HARNESSE(center)
    END_EFF()
    EFF(eff_rotational_vec2f)
        HARNESSE(center)
    END_EFF()
    EFF(eff_spiral_vec2f)
        HARNESSE(center)
        HARNESSE(angle)
    END_EFF()
    EFF(eff_fermat_spiral_vec2f)
        HARNESSE(c)
    END_EFF()
    EFF(eff_vortex_vec2f)
        HARNESSE(diameter)
        HARNESSE(soften)
        HARNESSE(intensity)
        HARNESSE(center_orig)
        READE(revolving)
        HARNESSE(velocity)
        HARNESSE(center_of_revolution)
    END_EFF()
    EFF(eff_turbulent_vec2f)
        HARNESSE(n)
        HARNESSE(bounds)
        HARNESSE(scale_factor)
        HARNESSE(min_diameter)
        HARNESSE(max_diameter)
        HARNESSE(min_soften)
        HARNESSE(max_soften)
        HARNESSE(min_intensity)
        HARNESSE(max_intensity)
        READE(intensity_direction)
        READE(revolving)
        HARNESSE(min_velocity)
        HARNESSE(max_velocity)
        READE(velocity_direction)
        HARNESSE(min_orbital_radius)
        HARNESSE(max_orbital_radius)
    END_EFF()
    EFF(eff_kaleidoscope_vec2f)
        HARNESSE(segments)
        HARNESSE(levels)
        HARNESSE(start)
        HARNESSE(level_start)
        HARNESSE(spin)
        HARNESSE(expand)
        HARNESSE(reflect)
        HARNESSE(reflect_levels)
    END_EFF()
    EFF(eff_radial_tile_vec2f)
        HARNESSE(segments)
        HARNESSE(levels)
        HARNESSE(offset_x)
        HARNESSE(offset_y)
        HARNESSE(spin)
        HARNESSE(expand)
        HARNESSE(zoom_x)
        HARNESSE(zoom_y)
        HARNESSE(reflect_x)
        HARNESSE(reflect_y)
    END_EFF()
    EFF(eff_radial_multiply_vec2f)
        HARNESSE(segments)
        HARNESSE(levels)
        HARNESSE(spin)
        HARNESSE(expand)
        HARNESSE(reflect)
        HARNESSE(reflect_levels)
    END_EFF()
    EFF(eff_theta_rotate_vec2f)
        HARNESSE(angle)
    END_EFF()
    EFF(eff_theta_swirl_vec2f)
        HARNESSE(amount)
    END_EFF()
    EFF(eff_theta_rings_vec2f)
        HARNESSE(n)
        HARNESSE(swirl)
        HARNESSE(alternate)
    END_EFF()
    EFF(eff_theta_waves_vec2f)
        HARNESSE(freq)
        HARNESSE(amp)
        HARNESSE(phase)
        HARNESSE(const_amp)
    END_EFF()
    EFF(eff_theta_saw_vec2f)
        HARNESSE(freq)
        HARNESSE(amp)
        HARNESSE(phase)
        HARNESSE(const_amp)
    END_EFF()
    EFF(eff_theta_compression_waves_vec2f)
        HARNESSE(freq)
        HARNESSE(amp)
        HARNESSE(phase)
        HARNESSE(const_amp)
    END_EFF()
    EFF(eff_position_fill_vec2f)
    END_EFF()

    // warp field effects
    EFF(eff_fill_warp_int)
        HARNESSE(vf_name)
        HARNESSE(relative)
        HARNESSE(extend)
    END_EFF()

    DEBUG("Finished reading effect " + name)
}

void scene_reader::read_queue(const json &j) {
    std::string name = "default";
    if (j.contains("name")) read(name, j["name"]);
    bool self_generated = false;
    if (j.contains("self_generated")) read(self_generated, j["self_generated"]);
    vec2i dim(512, 512);
    pixel_type ptype = pixel_type::PIXEL_UCOLOR;
    if (j.contains("type")) read(ptype, j["type"]);
    render_mode rmode = render_mode::MODE_STATIC;
    if (j.contains("mode")) read(rmode, j["mode"]);
    float relative_dim = 1.0;
    if (j.contains("relative_dim")) read(relative_dim, j["relative_dim"]);
    effect_list elist(name, self_generated, "none", dim, ptype, rmode, relative_dim);
    // DEBUG( "Reading queue " + .name )
    if (j.contains("effects")) for (std::string eff_name: j["effects"]) elist.effects.push_back(eff_name);;
    if (j.contains("source")) read_harness(j["source"], elist.source_name);
    if (j.contains("dim")) read_harness(j["dim"], elist.dim);

    // DEBUG( "queue source successfully read " )
    s.queue.push_back(elist);
    s.buffers[name] = elist.buf;
}

void scene_reader::read_widget_group(const json &j) {
    //    std::shared_ptr< widget_group > wg = std::make_shared< widget_group >();
    widget_group wg;

    if (j.contains("name")) read(wg.name, j["name"]);
    else
        ERROR("Widget group name missing\n")
    if (j.contains("label")) read(wg.label, j["label"]);
    if (j.contains("description")) read(wg.description, j["description"]);
    if (j.contains("conditions")) for (std::string condition: j["conditions"]) wg.add_condition(condition);
    if (j.contains("widgets")) for (std::string widget: j["widgets"]) wg.add_widget(widget);
    s.ui.widget_groups.push_back(wg);
}

void to_json(nlohmann::json &j, const interval_int &i) {
    j = nlohmann::json{
        i.min, i.max
    };
}

void to_json(nlohmann::json &j, const interval_float &i) {
    j = nlohmann::json{
        i.min, i.max
    };
}

void to_json(nlohmann::json &j, const direction4 &d) {
    switch (d) {
        case direction4::D4_UP:
            j = "up";
            break;
        case direction4::D4_RIGHT:
            j = "right";
            break;
        case direction4::D4_DOWN:
            j = "down";
            break;
        case direction4::D4_LEFT:
            j = "left";
            break;
        default:
            // Handle unexpected direction4 value
            j = nullptr; // Or any indication of an error/invalid value
            break;
    }
}

void to_json(nlohmann::json &j, const direction4_diagonal &d) {
    switch (d) {
        case direction4_diagonal::D4D_UPRIGHT:
            j = "up_right";
            break;
        case direction4_diagonal::D4D_DOWNRIGHT:
            j = "down_right";
            break;
        case direction4_diagonal::D4D_DOWNLEFT:
            j = "down_left";
            break;
        case direction4_diagonal::D4D_UPLEFT:
            j = "up_left";
            break;
        default:
            // Handle unexpected direction4_diagonal value
            j = nullptr; // Or any indication of an error/invalid value
            break;
    }
}

void to_json(nlohmann::json &j, const direction8 &d) {
    switch (d) {
        case direction8::D8_UP:
            j = "up";
            break;
        case direction8::D8_UPRIGHT:
            j = "up_right";
            break;
        case direction8::D8_RIGHT:
            j = "right";
            break;
        case direction8::D8_DOWNRIGHT:
            j = "down_right";
            break;
        case direction8::D8_DOWN:
            j = "down";
            break;
        case direction8::D8_DOWNLEFT:
            j = "down_left";
            break;
        case direction8::D8_LEFT:
            j = "left";
            break;
        case direction8::D8_UPLEFT:
            j = "up_left";
            break;
        default:
            // Handle unexpected direction8 value
            j = nullptr; // Or any indication of an error/invalid value
            break;
    }
}

void to_json(nlohmann::json &j, const image_extend &e) {
    switch (e) {
        case image_extend::SAMP_SINGLE:
            j = "single";
            break;
        case image_extend::SAMP_REPEAT:
            j = "repeat";
            break;
        case image_extend::SAMP_REFLECT:
            j = "reflect";
            break;
        default:
            j = "repeat";
            break;
    }
}

void to_json(nlohmann::json &j, const switch_fn &s) {
    j = nlohmann::json{
        {"label", s.label},
        {"description", s.description},
        {"value", *(s.value)},
        {"default_value", s.default_value},
        {"tool", s.tool == SWITCH_SWITCH ? "switch" : "checkbox"}
    };
}

void to_json(nlohmann::json &j, const widget_group &wg) {
    j = nlohmann::json{
        {"name", wg.name},
        {"label", wg.label},
        {"description", wg.description},
        {"conditions", wg.conditions},
        {"widgets", wg.widgets}
    };
}

void to_json(nlohmann::json &j, const box_blur_type &b) {
    switch (b) {
        case box_blur_type::BB_ORTHOGONAL:
            j = "orthogonal";
            break;
        case box_blur_type::BB_DIAGONAL:
            j = "diagonal";
            break;
        case box_blur_type::BB_ALL:
            j = "all";
            break;
        case box_blur_type::BB_CUSTOM:
            j = "custom";
            break;
        default:
            // Handle unexpected box_blur_method value
            j = nullptr; // Or any indication of an error/invalid value
            break;
    }
}

void to_json(nlohmann::json &j, const menu_type &m) {
    switch (m) {
        case MENU_PULL_DOWN:
            j = "pull_down";
            break;
        case MENU_RADIO:
            j = "radio";
            break;
        case MENU_IMAGE:
            j = "image";
            break;
        default:
            // Handle unexpected menu_type value
            j = nullptr; // Or any indication of an error/invalid value
            break;
    }
}

/*
void to_json(nlohmann::json& j, const std::vector<widget_group>& wg_vec) {
    for (const auto& wg : wg_vec) {
        nlohmann::json j_wg;
        to_json(j_wg, wg); // Serialize each widget_group to JSON
        j.push_back(j_wg); // Add it to the JSON array
    }
}
*/

std::string ullToHexString(unsigned long long value) {
    std::stringstream ss;
    ss << "0x" << std::hex << std::uppercase << value;
    return ss.str();
}

// Scene writer implementation
scene_writer::scene_writer(const scene &s_init) : s(s_init) {
}

nlohmann::json scene_writer::write_scene_json() const {
    nlohmann::json j;

    // Basic scene info
    j["name"] = s.name;
    j["title"] = s.name; // Use name as title if no separate title
    j["liveCamera"] = s.liveCamera;
    if (s.time_interval != s.default_time_interval) {
        j["time_interval"] = s.time_interval;
    }

    // Components
    j["images"] = write_images_json();
    j["functions"] = write_functions_json();
    j["effects"] = write_effects_json();
    j["queue"] = write_queue_json();
    j["widget groups"] = write_widget_groups_json();

    return j;
}

nlohmann::json scene_writer::write_images_json() const {
    nlohmann::json images = nlohmann::json::array();

    for (const auto &[name, buffer]: s.buffers) {
        // Only serialize uimage buffers as images
        if (std::holds_alternative<ubuf_ptr>(buffer)) {
            const auto &ubuf = std::get<ubuf_ptr>(buffer);
            if (ubuf && ubuf->has_image()) {
                nlohmann::json img;
                img["type"] = "uimage";
                img["filename"] = "lux_files/" + name + ".jpg"; // Use conventional path
                img["name"] = name;
                images.push_back(img);
            }
        }
    }

    return images;
}

nlohmann::json scene_writer::write_functions_json() const {
    nlohmann::json functions = nlohmann::json::array();

    for (const auto &[name, func]: s.functions) {
        functions.push_back(serialize_any_function(func));
    }

    return functions;
}

nlohmann::json scene_writer::serialize_any_function(const any_function &af) const {
    // Use your existing to_json implementation
    nlohmann::json j;
    j = af; // This calls your existing to_json overloads
    return j;
}

nlohmann::json scene_writer::write_effects_json() const {
    nlohmann::json effects = nlohmann::json::array();

    for (const auto &[name, effect]: s.effects) {
        nlohmann::json eff;
        eff["name"] = name;

        // Get the effect type name from the any_effect_fn
        std::string type_name = effect.get_type_name();
        eff["type"] = type_name;

        // Add effect-specific parameters
        effect.serialize_to_json(eff);

        effects.push_back(eff);
    }

    return effects;
}

nlohmann::json scene_writer::write_queue_json() const {
    nlohmann::json queue = nlohmann::json::array();

    for (const auto &q: s.queue) {
        nlohmann::json q_item;
        q_item["name"] = q.name;
        q_item["self_generated"] = q.self_generated;

        // Get pixel type name
        std::string pixel_type_name = "ucolor"; // Default
        // Check if buffer is valid and get pixel type
        if (std::holds_alternative<fbuf_ptr>(q.buf) ||
            std::holds_alternative<ubuf_ptr>(q.buf) ||
            std::holds_alternative<vbuf_ptr>(q.buf) ||
            std::holds_alternative<wbuf_ptr>(q.buf) ||
            std::holds_alternative<obuf_ptr>(q.buf)) {
            pixel_type_name = get_pixel_type_name(q.buf);
        }
        q_item["type"] = pixel_type_name;

        // Mode
        std::string mode_name = "static";
        if (q.rmode == MODE_ITERATIVE) mode_name = "iterative";
        else if (q.rmode == MODE_EPHEMERAL) mode_name = "ephemeral";
        q_item["mode"] = mode_name;

        if (!q.effects.empty()) {
            q_item["effects"] = q.effects;
        }

        if (*q.source_name != "none") {
            // Preserve harness structure for source_name
            if (!q.source_name.functions.empty()) {
                nlohmann::json source_json;
                nlohmann::json functions_array = nlohmann::json::array();
                for (const auto &func_obj: q.source_name.functions) {
                    functions_array.push_back(func_obj.name);
                }
                source_json["functions"] = functions_array;
                source_json["value"] = *q.source_name;
                q_item["source"] = source_json;
            } else {
                // Just the current value if no functions
                q_item["source"] = *q.source_name;
            }
        }

        if (q.relative_dim != 1.0f) {
            q_item["relative_dim"] = q.relative_dim;
        }

        // Preserve harness structure for dim if it has functions
        if (!q.dim.functions.empty()) {
            nlohmann::json dim_json;
            nlohmann::json functions_array = nlohmann::json::array();
            for (const auto &func_obj: q.dim.functions) {
                functions_array.push_back(func_obj.name);
            }
            dim_json["functions"] = functions_array;
            dim_json["value"] = *q.dim;
            q_item["dim"] = dim_json;
        }

        queue.push_back(q_item);
    }

    return queue;
}

nlohmann::json scene_writer::write_widget_groups_json() const {
    nlohmann::json groups = nlohmann::json::array();

    for (const auto &wg: s.ui.widget_groups) {
        groups.push_back(wg); // Use your existing to_json for widget_group
    }

    return groups;
}

std::string ulToHexString(unsigned long value) {
    std::stringstream ss;
    ss << "0x" << std::hex << std::uppercase << value;
    return ss.str();
}

// Helper function to serialize harness values with function references
template<typename T>
nlohmann::json serialize_harness(const harness<T> &harness_field, const std::string &field_name) {
    if (!harness_field.functions.empty()) {
        // Preserve harness structure with function references
        nlohmann::json result;
        nlohmann::json functions_array = nlohmann::json::array();
        for (const auto &func_obj: harness_field.functions) {
            functions_array.push_back(func_obj.name);
        }
        result["functions"] = functions_array;
        result["value"] = *harness_field;
        return result;
    } else {
        // Just the current value if no functions
        return *harness_field;
    }
}

void to_json(nlohmann::json &j, const any_function &af) {
    std::visit(overloaded{
                   [&](const any_fn<bool> &wrapper) {
                       std::visit(overloaded{
                                      [&](const std::shared_ptr<switch_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "switch_fn"},
                                              {"label", fn->label},
                                              {"description", fn->description},
                                              {"default_value", fn->default_value},
                                              {"tool", fn->tool == SWITCH_SWITCH ? "switch" : "checkbox"},
                                              {"affects_widget_groups", fn->affects_widget_groups}
                                          };

                                          // Add harness function references if they exist, otherwise just the runtime value
                                          if (!fn->value.functions.empty()) {
                                              nlohmann::json harness_json;
                                              nlohmann::json functions_array = nlohmann::json::array();
                                              for (const auto &func_obj: fn->value.functions) {
                                                  functions_array.push_back(func_obj.name);
                                              }
                                              harness_json["functions"] = functions_array;
                                              harness_json["value"] = *fn->value;
                                              j["value"] = harness_json;
                                          } else {
                                              // No functions, just store the current runtime value
                                              j["value"] = *fn->value;
                                          }
                                      },
                                      [&](const std::shared_ptr<widget_switch_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "widget_switch_fn"},
                                              {"label", fn->label},
                                              {"description", fn->description},
                                              {"switcher", fn->switcher},
                                              {"widget", fn->widget}
                                          };
                                      },
                                      [&](const std::shared_ptr<identity_fn<bool> > &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "identity_fn<bool>"},
                                              {"description", "Boolean identity function"}
                                          };
                                      },
                                      [&](const std::shared_ptr<initial_element_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "initial_element_fn"},
                                              {"description", "Initial element function"}
                                          };
                                      },
                                      [&](const std::shared_ptr<following_element_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "following_element_fn"},
                                              {"description", "Following element function"}
                                          };
                                      },
                                      [&](const std::shared_ptr<top_level_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "top_level_fn"},
                                              {"description", "Top level function"}
                                          };
                                      },
                                      [&](const std::shared_ptr<lower_level_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "lower_level_fn"},
                                              {"description", "Lower level function"}
                                          };
                                      },

                                      [&](const std::shared_ptr<random_toggle> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "random_toggle"},
                                              {"description", "Random toggle function"},
                                              {"p", serialize_harness(fn->p, "p")},
                                              {"enabled", serialize_harness(fn->enabled, "enabled")}
                                          };
                                      },
                                      [&](const std::shared_ptr<random_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "random_fn"},
                                              {"description", "Random function"}
                                          };
                                      },
                                      [&](const std::shared_ptr<random_sticky_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "random_sticky_fn"},
                                              {"description", "Random sticky function"}
                                          };
                                      },
                                      [&](const std::shared_ptr<equal_float_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "equal_float_fn"},
                                              {"description", "Float equality function"},
                                              {"a", serialize_harness(fn->a, "a")},
                                              {"b", serialize_harness(fn->b, "b")}
                                          };
                                      },
                                      [&](const std::shared_ptr<equal_vec2f_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "equal_vec2f_fn"},
                                              {"description", "Vec2f equality function"},
                                              {"a", serialize_harness(fn->a, "a")},
                                              {"b", serialize_harness(fn->b, "b")}
                                          };
                                      },
                                      [&](const std::shared_ptr<equal_int_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "equal_int_fn"},
                                              {"description", "Integer equality function"},
                                              {"a", serialize_harness(fn->a, "a")},
                                              {"b", serialize_harness(fn->b, "b")}
                                          };
                                      },
                                      [&](const std::shared_ptr<equal_vec2i_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "equal_vec2i_fn"},
                                              {"description", "Vec2i equality function"},
                                              {"a", serialize_harness(fn->a, "a")},
                                              {"b", serialize_harness(fn->b, "b")}
                                          };
                                      },
                                      [&](const std::shared_ptr<equal_frgb_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "equal_frgb_fn"},
                                              {"description", "Frgb equality function"},
                                              {"a", serialize_harness(fn->a, "a")},
                                              {"b", serialize_harness(fn->b, "b")}
                                          };
                                      },
                                      [&](const std::shared_ptr<equal_ucolor_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "equal_ucolor_fn"},
                                              {"description", "Ucolor equality function"},
                                              {"a", serialize_harness(fn->a, "a")},
                                              {"b", serialize_harness(fn->b, "b")}
                                          };
                                      },
                                      [&](const std::shared_ptr<equal_string_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "equal_string_fn"},
                                              {"description", "String equality function"},
                                              {"a", serialize_harness(fn->a, "a")},
                                              {"b", serialize_harness(fn->b, "b")}
                                          };
                                      },
                                      [&](const std::shared_ptr<equal_bool_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "equal_bool_fn"},
                                              {"description", "Boolean equality function"},
                                              {"a", serialize_harness(fn->a, "a")},
                                              {"b", serialize_harness(fn->b, "b")}
                                          };
                                      },
                                      [&](const std::shared_ptr<equal_direction4_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "equal_direction4_fn"},
                                              {"description", "Direction4 equality function"},
                                              {"a", serialize_harness(fn->a, "a")},
                                              {"b", serialize_harness(fn->b, "b")}
                                          };
                                      },
                                      [&](const std::shared_ptr<equal_direction4_diagonal_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "equal_direction4_diagonal_fn"},
                                              {"description", "Direction4 diagonal equality function"},
                                              {"a", serialize_harness(fn->a, "a")},
                                              {"b", serialize_harness(fn->b, "b")}
                                          };
                                      },
                                      [&](const std::shared_ptr<equal_direction8_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "equal_direction8_fn"},
                                              {"description", "Direction8 equality function"},
                                              {"a", serialize_harness(fn->a, "a")},
                                              {"b", serialize_harness(fn->b, "b")}
                                          };
                                      },
                                      [&](const std::shared_ptr<mousedown_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "mousedown_fn"},
                                              {"description", "Mouse down function"}
                                          };
                                      },
                                      [&](const std::shared_ptr<mouseover_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "mouseover_fn"},
                                              {"description", "Mouse over function"}
                                          };
                                      },
                                      [&](const std::shared_ptr<mouseclick_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "mouseclick_fn"},
                                              {"description", "Mouse click function"}
                                          };
                                      }
                                  }, wrapper.any_fn_ptr);
                   },
                   [&](const any_fn<int> &wrapper) {
                       std::visit(overloaded{
                                      [&](const std::shared_ptr<slider_int> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "slider_int"},
                                              {"label", fn->label},
                                              {"description", fn->description},
                                              {"min", fn->min},
                                              {"max", fn->max},
                                              {"default_value", fn->default_value},
                                              {"step", fn->step},
                                              {"value", *fn->value}
                                          };

                                          // Add harness function references if they exist
                                          if (!fn->value.functions.empty()) {
                                              nlohmann::json harness_json;
                                              harness_json["functions"] = fn->value.functions;
                                              j["value"] = harness_json;
                                          }
                                      },
                                      [&](const std::shared_ptr<menu_int> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "menu_int"},
                                              {"label", fn->label},
                                              {"description", fn->description},
                                              {"default_choice", fn->default_choice},
                                              {"tool", fn->tool},
                                              {"items", fn->items},
                                              {"affects_widget_groups", fn->affects_widget_groups},
                                              {"rerender", fn->rerender}
                                          };

                                          // Add harness function references if they exist, otherwise just the runtime value
                                          if (!fn->choice.functions.empty()) {
                                              nlohmann::json harness_json;
                                              nlohmann::json functions_array = nlohmann::json::array();
                                              for (const auto &func_obj: fn->choice.functions) {
                                                  functions_array.push_back(func_obj.name);
                                              }
                                              harness_json["functions"] = functions_array;
                                              harness_json["value"] = *fn->choice;
                                              j["choice"] = harness_json;
                                          } else {
                                              // No functions, just store the current runtime value
                                              j["choice"] = *fn->choice;
                                          }
                                      },
                                      [&](const std::shared_ptr<multi_direction8_picker> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "multi_direction_picker"},
                                              {"label", fn->label},
                                              {"description", fn->description},
                                              {"value", fn->value},
                                              {"default_value", fn->default_value}
                                          };
                                      },
                                      [&](const std::shared_ptr<custom_blur_picker> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "custom_blur_picker"},
                                              {"label", fn->label},
                                              {"description", fn->description},
                                              {"pickers", fn->pickers}
                                          };
                                      },
                                      [&](const std::shared_ptr<range_slider_int> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "range_slider_int"},
                                              {"label", fn->label},
                                              {"description", fn->description},
                                              {"min", fn->min},
                                              {"max", fn->max},
                                              {"step", fn->step},
                                              {"value", fn->value},
                                              {"default_value", fn->default_value}
                                          };
                                      },
                                      [&](const std::shared_ptr<identity_int> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "identity_int"},
                                              {"description", "Integer identity function"}
                                          };
                                      },
                                      [&](const std::shared_ptr<adder_int> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "adder_int"},
                                              {"description", "Integer addition function"},
                                              {"r", serialize_harness(fn->r, "r")}
                                          };
                                      },
                                      [&](const std::shared_ptr<generator_int> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "generator_int"},
                                              {"description", "Integer generator function"},
                                              {
                                                  "distribution",
                                                  fn->distribution == PROB_UNIFORM
                                                      ? "uniform"
                                                      : fn->distribution == PROB_NORMAL
                                                            ? "normal"
                                                            : fn->distribution == PROB_LOG_NORMAL
                                                                  ? "log_normal"
                                                                  : fn->distribution == PROB_GEOMETRIC
                                                                        ? "geometric"
                                                                        : "uniform"
                                              },
                                              {"p", serialize_harness(fn->p, "p")},
                                              {"a", serialize_harness(fn->a, "a")},
                                              {"b", serialize_harness(fn->b, "b")},
                                              {"min", serialize_harness(fn->min, "min")},
                                              {"max", serialize_harness(fn->max, "max")},
                                              {"enabled", serialize_harness(fn->enabled, "enabled")}
                                          };
                                      },
                                      [&](const std::shared_ptr<tweaker_int> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "tweaker_int"},
                                              {"description", "Integer tweaker function"},
                                              {"p", serialize_harness(fn->p, "p")},
                                              {"amount", serialize_harness(fn->amount, "amount")},
                                              {"enabled", serialize_harness(fn->enabled, "enabled")}
                                          };
                                      },
                                      [&](const auto &fn) {
                                          // Placeholder for other types
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "unimplemented json output for int function"}
                                              // Replace with actual type identification if needed
                                              // Other placeholder fields...
                                          };
                                      }
                                  }, wrapper.any_fn_ptr);
                   },
                   [&](const any_fn<float> &wrapper) {
                       std::visit(overloaded {
                                      [&](const std::shared_ptr<slider_float> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "slider_float"},
                                              {"label", fn->label},
                                              {"description", fn->description},
                                              {"min", fn->min},
                                              {"max", fn->max},
                                              {"default_value", fn->default_value},
                                              {"step", fn->step}
                                          };

                                          // Add harness function references if they exist, otherwise just the runtime value
                                          if (!fn->value.functions.empty()) {
                                              nlohmann::json harness_json;
                                              nlohmann::json functions_array = nlohmann::json::array();
                                              for (const auto &func_obj: fn->value.functions) {
                                                  functions_array.push_back(func_obj.name);
                                              }
                                              harness_json["functions"] = functions_array;
                                              harness_json["value"] = *fn->value;
                                              j["value"] = harness_json;
                                          } else {
                                              // No functions, just store the current runtime value
                                              j["value"] = *fn->value;
                                          }
                                      },
                                      [&](const std::shared_ptr<audio_adder_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "audio_adder_fn"},
                                              {"volume_channel", *fn->volume_channel},
                                              {"volume_weight", *fn->volume_weight},
                                              {"volume_sensitivity", *fn->volume_sensitivity},
                                              {"bass_channel", *fn->bass_channel},
                                              {"bass_weight", *fn->bass_weight},
                                              {"bass_sensitivity", *fn->bass_sensitivity},
                                              {"mid_channel", *fn->mid_channel},
                                              {"mid_weight", *fn->mid_weight},
                                              {"mid_sensitivity", *fn->mid_sensitivity},
                                              {"high_channel", *fn->high_channel},
                                              {"high_weight", *fn->high_weight},
                                              {"high_sensitivity", *fn->high_sensitivity},
                                              {"offset", *fn->offset},
                                              {"global_sensitivity", *fn->global_sensitivity}
                                          };
                                      },
                                      [&](const std::shared_ptr<identity_float> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "identity_float"},
                                              {"description", "Float identity function"}
                                          };
                                      },
                                      [&](const std::shared_ptr<adder_float> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "adder_float"},
                                              {"description", "Float addition function"},
                                              {"r", serialize_harness(fn->r, "r")}
                                          };
                                      },
                                      [&](const std::shared_ptr<tweaker_float> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "tweaker_float"},
                                              {"description", "Float tweaker function"},
                                              {"p", serialize_harness(fn->p, "p")},
                                              {"amount", serialize_harness(fn->amount, "amount")},
                                              {"enabled", serialize_harness(fn->enabled, "enabled")}
                                          };
                                      },
                                      [&](const std::shared_ptr<integrator_float> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "integrator_float"},
                                              {"description", "Float integrator function"},
                                              {"delta", serialize_harness(fn->delta, "delta")},
                                              {"scale", serialize_harness(fn->scale, "scale")},
                                              {"val", fn->val}
                                          };
                                      },
                                      [&](const std::shared_ptr<log_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "log_fn"},
                                              {"description", "Logarithmic function"},
                                              {"scale", serialize_harness(fn->scale, "scale")},
                                              {"shift", serialize_harness(fn->shift, "shift")}
                                          };
                                      },
                                      [&](const std::shared_ptr<ratio_float> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "ratio_float"},
                                              {"description", "Float ratio function"},
                                              {"r", serialize_harness(fn->r, "r")}
                                          };
                                      },
                                      [&](const std::shared_ptr<index_param<float> > &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "index_param<float>"},
                                              {"description", "Float index parameter"}
                                          };
                                      },
                                      [&](const std::shared_ptr<scale_param<float> > &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "scale_param<float>"},
                                              {"description", "Float scale parameter"}
                                          };
                                      },
                                      [&](const std::shared_ptr<time_param<float> > &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "time_param<float>"},
                                              {"description", "Float time parameter"}
                                          };
                                      },
                                      [&](const std::shared_ptr<range_slider_float> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "range_slider_float"},
                                              {"label", fn->label},
                                              {"description", fn->description},
                                              {"min", fn->min},
                                              {"max", fn->max},
                                              {"step", fn->step},
                                              {"value", fn->value},
                                              {"default_value", fn->default_value}
                                          };
                                      },
                                      [&](const std::shared_ptr<time_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "time_fn"},
                                              {"description", "Time function"}
                                          };
                                      },
                                      [&](const std::shared_ptr<wiggle> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "wiggle"},
                                              {"description", "Wiggle function"}
                                          };
                                      },
                                      // [&](const std::shared_ptr<generator<float> > &fn) {
                                      //     j = nlohmann::json{
                                      //         {"name", wrapper.name},
                                      //         {"type", "generator_float"},
                                      //         {"description", "Float generator function"},
                                      //         {
                                      //             "distribution",
                                      //             fn->distribution == PROB_UNIFORM
                                      //                 ? "uniform"
                                      //                 : fn->distribution == PROB_NORMAL
                                      //                       ? "normal"
                                      //                       : fn->distribution == PROB_LOG_NORMAL
                                      //                             ? "log_normal"
                                      //                             : fn->distribution == PROB_GEOMETRIC
                                      //                                   ? "geometric"
                                      //                                   : "uniform"
                                      //         },
                                      //         {"p", serialize_harness(fn->p, "p")},
                                      //         {"a", serialize_harness(fn->a, "a")},
                                      //         {"b", serialize_harness(fn->b, "b")},
                                      //         {"enabled", serialize_harness(fn->enabled, "enabled")}
                                      //     };
                                      // },
                                      // [&](const std::shared_ptr<tweaker<float> > &fn) {
                                      //     j = nlohmann::json{
                                      //         {"name", wrapper.name},
                                      //         {"type", "tweaker_float"},
                                      //         {"description", "Float tweaker function"},
                                      //         {"p", serialize_harness(fn->p, "p")},
                                      //         {"amount", serialize_harness(fn->amount, "amount")},
                                      //         {"enabled", serialize_harness(fn->enabled, "enabled")}
                                      //     };
                                      // },
                                      // [&](const std::shared_ptr<integrator<float> > &fn) {
                                      //     j = nlohmann::json{
                                      //         {"name", wrapper.name},
                                      //         {"type", "integrator_float"},
                                      //         {"description", "Float integrator function"},
                                      //         {"delta", serialize_harness(fn->delta, "delta")},
                                      //         {"scale", serialize_harness(fn->scale, "scale")},
                                      //         {"val", fn->val}
                                      //     };
                                      // },
                                      // [&](const std::shared_ptr<adder<float> > &fn) {
                                      //     j = nlohmann::json{
                                      //         {"name", wrapper.name},
                                      //         {"type", "adder_float"},
                                      //         {"description", "Float addition function"},
                                      //         {"r", serialize_harness(fn->r, "r")}
                                      //     };
                                      // },
                                      [&](const std::shared_ptr<generator_float> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "generator_float"},
                                              {"description", "Float generator function"},
                                              {
                                                  "distribution",
                                                  fn->distribution == PROB_UNIFORM
                                                      ? "uniform"
                                                      : fn->distribution == PROB_NORMAL
                                                            ? "normal"
                                                            : fn->distribution == PROB_LOG_NORMAL
                                                                  ? "log_normal"
                                                                  : fn->distribution == PROB_GEOMETRIC
                                                                        ? "geometric"
                                                                        : "uniform"
                                              },
                                              {"p", serialize_harness(fn->p, "p")},
                                              {"a", serialize_harness(fn->a, "a")},
                                              {"b", serialize_harness(fn->b, "b")},
                                              {"enabled", serialize_harness(fn->enabled, "enabled")}
                                          };
                                      },
                                      // [&](const std::shared_ptr<ratio<float> > &fn) {
                                      //     j = nlohmann::json{
                                      //         {"name", wrapper.name},
                                      //         {"type", "ratio_float"},
                                      //         {"description", "Float ratio function"},
                                      //         {"r", serialize_harness(fn->r, "r")}
                                      //     };
                                      // },
                                      [&](const auto &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "unimplemented_float_function"}
                                          };
                                      }
                                  }, wrapper.any_fn_ptr);
                   },
                   [&](const any_fn<funk_factor> &wrapper) {
                       std::visit(overloaded{
                                      [&](const std::shared_ptr<funk_factor_picker> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "funk_factor_picker"},
                                              {"label", fn->label},
                                              {"description", fn->description},
                                              {"value", ullToHexString(fn->value)},
                                              {"default_value", ullToHexString(fn->default_value)}
                                          };
                                      },
                                      [&](const std::shared_ptr<identity_funk_factor> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "identity_funk_factor"},
                                              {"description", "Funk factor identity function"}
                                          };
                                      },
                                      [&](const std::shared_ptr<adder_funk_factor> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "adder_funk_factor"},
                                              {"description", "Funk factor addition function"}
                                          };
                                      },
                                      [&](const auto &fn) {
                                          // Placeholder for other types
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "unimplemented funk_factor function"}
                                              // Replace with actual type identification if needed
                                              // Other placeholder fields...
                                          };
                                      }
                                  }, wrapper.any_fn_ptr);
                   },
                   [&](const any_fn<interval_int> &wrapper) {
                       std::visit(overloaded{
                                      [&](const std::shared_ptr<range_slider_int> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "range_slider_int"},
                                              {"label", fn->label},
                                              {"description", fn->description},
                                              {"min", fn->min},
                                              {"max", fn->max},
                                              {"default_value", fn->default_value},
                                              {"step", fn->step},
                                              {"value", fn->value}
                                          };
                                      },
                                      [&](const std::shared_ptr<identity_interval_int> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "identity_interval_int"},
                                              {"description", "Interval int identity function"}
                                          };
                                      },
                                      [&](const auto &fn) {
                                          // Placeholder for other types
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "unimplemented interval_int function"}
                                              // Replace with actual type identification if needed
                                              // Other placeholder fields...
                                          };
                                      }
                                  }, wrapper.any_fn_ptr);
                   },
                   [&](const any_fn<interval_float> &wrapper) {
                       std::visit(overloaded{
                                      [&](const std::shared_ptr<range_slider_float> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "range_slider_float"},
                                              {"label", fn->label},
                                              {"description", fn->description},
                                              {"min", fn->min},
                                              {"max", fn->max},
                                              {"default_value", fn->default_value},
                                              {"step", fn->step},
                                              {"value", fn->value}
                                          };
                                      },
                                      [&](const std::shared_ptr<identity_interval_float> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "identity_interval_float"},
                                              {"description", "Interval float identity function"}
                                          };
                                      },
                                      [&](const auto &fn) {
                                          // Placeholder for other types
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "unimplemented interval_float function"}
                                              // Replace with actual type identification if needed
                                              // Other placeholder fields...
                                          };
                                      }
                                  }, wrapper.any_fn_ptr);
                   },
                   [&](const any_fn<vec2f> &wrapper) {
                       std::visit(overloaded{
                                      [&](const std::shared_ptr<identity_vec2f> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "identity_vec2f"},
                                              {"description", "Vec2f identity function"}
                                          };
                                      },
                                      [&](const std::shared_ptr<adder_vec2f> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "adder_vec2f"},
                                              {"description", "Vec2f addition function"},
                                              {"r", serialize_harness(fn->r, "r")}
                                          };
                                      },
                                      [&](const std::shared_ptr<ratio_vec2f> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "ratio_vec2f"},
                                              {"description", "Vec2f ratio function"},
                                              {"r", serialize_harness(fn->r, "r")}
                                          };
                                      },
                                      [&](const std::shared_ptr<mouse_pos_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "mouse_pos_fn"},
                                              {"description", "Mouse position function"}
                                          };
                                      },
                                      [&](const auto &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "unimplemented vec2f function"}
                                          };
                                      }
                                  }, wrapper.any_fn_ptr);
                   },
                   [&](const any_fn<vec2i> &wrapper) {
                       std::visit(overloaded{
                                      [&](const std::shared_ptr<identity_vec2i> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "identity_vec2i"},
                                              {"description", "Vec2i identity function"}
                                          };
                                      },
                                      [&](const std::shared_ptr<adder_vec2i> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "adder_vec2i"},
                                              {"description", "Vec2i addition function"},
                                              {"r", serialize_harness(fn->r, "r")}
                                          };
                                      },
                                      [&](const std::shared_ptr<buffer_dim_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "buffer_dim_fn"},
                                              {"description", "Buffer dimension function"},
                                              {"buf_name", serialize_harness(fn->buf_name, "buf_name")}
                                          };
                                      },
                                      [&](const std::shared_ptr<mouse_pix_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "mouse_pix_fn"},
                                              {"description", "Mouse pixel function"}
                                          };
                                      },
                                      [&](const auto &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "unimplemented vec2i function"}
                                          };
                                      }
                                  }, wrapper.any_fn_ptr);
                   },
                   [&](const any_fn<frgb> &wrapper) {
                       std::visit(overloaded{
                                      [&](const std::shared_ptr<identity_frgb> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "identity_frgb"},
                                              {"description", "Frgb identity function"}
                                          };
                                      },
                                      [&](const std::shared_ptr<adder_frgb> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "adder_frgb"},
                                              {"description", "Frgb addition function"},
                                              {"r", serialize_harness(fn->r, "r")}
                                          };
                                      },
                                      [&](const auto &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "unimplemented frgb function"}
                                          };
                                      }
                                  }, wrapper.any_fn_ptr);
                   },
                   [&](const any_fn<ucolor> &wrapper) {
                       std::visit(overloaded{
                                      [&](const std::shared_ptr<ucolor_picker> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "ucolor_picker"},
                                              {"label", fn->label},
                                              {"description", fn->description},
                                              {"value", ulToHexString(fn->value)},
                                              {"default_value", ulToHexString(fn->default_value)}
                                          };
                                      },
                                      [&](const std::shared_ptr<identity_ucolor> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "identity_ucolor"},
                                              {"description", "Ucolor identity function"}
                                          };
                                      },
                                      [&](const std::shared_ptr<adder_ucolor> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "adder_ucolor"},
                                              {"description", "Ucolor addition function"},
                                              {"r", serialize_harness(fn->r, "r")}
                                          };
                                      },
                                      [&](const auto &fn) {
                                          // Placeholder for other types
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "unimplemented ucolor function"}
                                              // Replace with actual type identification if needed
                                              // Other placeholder fields...
                                          };
                                      }
                                  }, wrapper.any_fn_ptr);
                   },
                   [&](const any_fn<std::string> &wrapper) {
                       std::visit(overloaded{
                                      [&](const std::shared_ptr<menu_string> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "menu_string"},
                                              {"label", fn->label},
                                              {"description", fn->description},
                                              {"default_choice", fn->default_choice},
                                              {"tool", fn->tool},
                                              {"items", fn->items},
                                              {"affects_widget_groups", fn->affects_widget_groups},
                                              {"rerender", fn->rerender}
                                          };

                                          // Add harness function references if they exist, otherwise just the runtime value
                                          if (!fn->choice.functions.empty()) {
                                              nlohmann::json harness_json;
                                              nlohmann::json functions_array = nlohmann::json::array();
                                              for (const auto &func_obj: fn->choice.functions) {
                                                  functions_array.push_back(func_obj.name);
                                              }
                                              harness_json["functions"] = functions_array;
                                              harness_json["value"] = *fn->choice;
                                              j["choice"] = harness_json;
                                          } else {
                                              // No functions, just store the current runtime value  
                                              j["choice"] = *fn->choice;
                                          }
                                      },
                                      [&](const std::shared_ptr<identity_string> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "identity_string"},
                                              {"description", "String identity function"}
                                          };
                                      },
                                      [&](const auto &fn) {
                                          // Placeholder for other types
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "unimplemented string function"}
                                              // Replace with actual type identification if needed
                                              // Other placeholder fields...
                                          };
                                      },
                                  }, wrapper.any_fn_ptr);
                   },
                   [&](const any_fn<bb2f> &wrapper) {
                       std::visit(overloaded{
                                      [&](const std::shared_ptr<identity_bb2f> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "identity_bb2f"},
                                              {"description", "Bounding box identity function"}
                                          };
                                      },
                                      [&](const auto &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "unimplemented bb2f function"}
                                          };
                                      }
                                  }, wrapper.any_fn_ptr);
                   },
                   [&](const any_fn<direction4> &wrapper) {
                       std::visit(overloaded{
                                      [&](const std::shared_ptr<direction_picker_4> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "direction_picker_4"},
                                              {"label", fn->label},
                                              {"description", fn->description},
                                              {"value", fn->value},
                                              {"default_value", fn->default_value}
                                          };
                                      },
                                      [&](const std::shared_ptr<identity_direction4> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "identity_direction4"},
                                              {"description", "Direction4 identity function"}
                                          };
                                      },
                                      [&](const auto &fn) {
                                          // Placeholder for other types
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "unimplemented direction4 function"}
                                              // Replace with actual type identification if needed
                                              // Other placeholder fields...
                                          };
                                      }
                                  }, wrapper.any_fn_ptr);
                   },
                   [&](const any_fn<direction4_diagonal> &wrapper) {
                       std::visit(overloaded{
                                      [&](const std::shared_ptr<direction_picker_4_diagonal> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "direction_picker_4_diagonal"},
                                              {"label", fn->label},
                                              {"description", fn->description},
                                              {"value", fn->value},
                                              {"default_value", fn->default_value}
                                          };
                                      },
                                      [&](const std::shared_ptr<identity_direction4_diagonal> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "identity_direction4_diagonal"},
                                              {"description", "Direction4 diagonal identity function"}
                                          };
                                      },
                                      [&](const auto &fn) {
                                          // Placeholder for other types
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "unimplemented direction4_diagonal function"}
                                              // Replace with actual type identification if needed
                                              // Other placeholder fields...
                                          };
                                      }
                                  }, wrapper.any_fn_ptr);
                   },
                   [&](const any_fn<direction8> &wrapper) {
                       std::visit(overloaded{
                                      [&](const std::shared_ptr<direction_picker_8> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "direction_picker_8"},
                                              {"label", fn->label},
                                              {"description", fn->description},
                                              {"value", fn->value},
                                              {"default_value", fn->default_value}
                                          };
                                      },
                                      [&](const std::shared_ptr<identity_direction8> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "identity_direction8"},
                                              {"description", "Direction8 identity function"}
                                          };
                                      },
                                      [&](const auto &fn) {
                                          // Placeholder for other types
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "unimplemented direction8 function"}
                                              // Replace with actual type identification if needed
                                              // Other placeholder fields...
                                          };
                                      }
                                  }, wrapper.any_fn_ptr);
                   },
                   [&](const any_fn<box_blur_type> &wrapper) {
                       std::visit(overloaded{
                                      [&](const std::shared_ptr<box_blur_picker> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "box_blur_picker"},
                                              {"label", fn->label},
                                              {"description", fn->description},
                                              {"value", fn->value},
                                              {"default_value", fn->default_value}
                                          };
                                      },
                                      [&](const std::shared_ptr<identity_box_blur_type> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "identity_box_blur_type"},
                                              {"description", "Box blur type identity function"}
                                          };
                                      },
                                      [&](const auto &fn) {
                                          // Placeholder for other types
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "unimplemented box_blur_type function"}
                                              // Replace with actual type identification if needed
                                              // Other placeholder fields...
                                          };
                                      }
                                  }, wrapper.any_fn_ptr);
                   },
                   [&](const any_fn<image_extend> &wrapper) {
                       std::visit(overloaded{
                                      [&](const std::shared_ptr<identity_image_extend> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "identity_image_extend"},
                                              {"description", "Image extend identity function"}
                                          };
                                      },
                                      [&](const std::shared_ptr<image_extend_picker> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "image_extend_picker"},
                                              {"label", fn->label},
                                              {"description", fn->description},
                                              {"value", fn->value},
                                              {"default_value", fn->default_value}
                                          };
                                      },
                                      [&](const auto &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "unimplemented gen function"}
                                          };
                                      }
                                  }, wrapper.any_fn_ptr);
                   },
                   [&](const any_condition_fn &wrapper) {
                       std::visit(overloaded{
                                      [&](const std::shared_ptr<switch_condition> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "switch_condition"},
                                              {"label", fn->label},
                                              {"description", fn->description},
                                              {"default_value", fn->default_value},
                                              {"tool", fn->tool == SWITCH_SWITCH ? "switch" : "checkbox"},
                                              {"affects_widget_groups", fn->affects_widget_groups}
                                          };

                                          // Add harness function references if they exist
                                          if (!fn->value.functions.empty()) {
                                              nlohmann::json harness_json;
                                              nlohmann::json functions_array = nlohmann::json::array();
                                              for (const auto &func_obj: fn->value.functions) {
                                                  functions_array.push_back(func_obj.name);
                                              }
                                              harness_json["functions"] = functions_array;
                                              harness_json["value"] = *fn->value;
                                              j["value"] = harness_json;
                                          } else {
                                              j["value"] = *fn->value;
                                          }
                                      },
                                      [&](const std::shared_ptr<widget_switch_condition> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "widget_switch_condition"},
                                              {"label", fn->label},
                                              {"description", fn->description},
                                              {"switcher", fn->switcher},
                                              {"widget", fn->widget}
                                          };
                                      },
                                      [&](const std::shared_ptr<random_condition> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "random_condition"},
                                              {"description", "Random condition function"},
                                              {"p", serialize_harness(fn->p, "p")}
                                          };
                                      },
                                      [&](const std::shared_ptr<random_sticky_condition> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "random_sticky_condition"},
                                              {"description", "Random sticky condition function"},
                                              {"p_start", serialize_harness(fn->p_start, "p_start")},
                                              {"p_change_true", serialize_harness(fn->p_change_true, "p_change_true")},
                                              {
                                                  "p_change_false",
                                                  serialize_harness(fn->p_change_false, "p_change_false")
                                              }
                                          };
                                      },
                                      [&](const std::shared_ptr<equal_float_condition> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "equal_float_condition"},
                                              {"description", "Float equality condition"},
                                              {"a", serialize_harness(fn->a, "a")},
                                              {"b", serialize_harness(fn->b, "b")}
                                          };
                                      },
                                      [&](const std::shared_ptr<equal_vec2f_condition> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "equal_vec2f_condition"},
                                              {"description", "Vec2f equality condition"},
                                              {"a", serialize_harness(fn->a, "a")},
                                              {"b", serialize_harness(fn->b, "b")}
                                          };
                                      },
                                      [&](const std::shared_ptr<equal_int_condition> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "equal_int_condition"},
                                              {"description", "Integer equality condition"},
                                              {"a", serialize_harness(fn->a, "a")},
                                              {"b", serialize_harness(fn->b, "b")}
                                          };
                                      },
                                      [&](const std::shared_ptr<equal_vec2i_condition> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "equal_vec2i_condition"},
                                              {"description", "Vec2i equality condition"},
                                              {"a", serialize_harness(fn->a, "a")},
                                              {"b", serialize_harness(fn->b, "b")}
                                          };
                                      },
                                      [&](const std::shared_ptr<equal_frgb_condition> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "equal_frgb_condition"},
                                              {"description", "Frgb equality condition"},
                                              {"a", serialize_harness(fn->a, "a")},
                                              {"b", serialize_harness(fn->b, "b")}
                                          };
                                      },
                                      [&](const std::shared_ptr<equal_ucolor_condition> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "equal_ucolor_condition"},
                                              {"description", "Ucolor equality condition"},
                                              {"a", serialize_harness(fn->a, "a")},
                                              {"b", serialize_harness(fn->b, "b")}
                                          };
                                      },
                                      [&](const std::shared_ptr<equal_string_condition> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "equal_string_condition"},
                                              {"description", "String equality condition"},
                                              {"a", serialize_harness(fn->a, "a")},
                                              {"b", serialize_harness(fn->b, "b")}
                                          };
                                      },
                                      [&](const std::shared_ptr<equal_bool_condition> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "equal_bool_condition"},
                                              {"description", "Boolean equality condition"},
                                              {"a", serialize_harness(fn->a, "a")},
                                              {"b", serialize_harness(fn->b, "b")}
                                          };
                                      },
                                      [&](const std::shared_ptr<equal_direction4_condition> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "equal_direction4_condition"},
                                              {"description", "Direction4 equality condition"},
                                              {"a", serialize_harness(fn->a, "a")},
                                              {"b", serialize_harness(fn->b, "b")}
                                          };
                                      },
                                      [&](const std::shared_ptr<equal_direction4_diagonal_condition> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "equal_direction4_diagonal_condition"},
                                              {"description", "Direction4 diagonal equality condition"},
                                              {"a", serialize_harness(fn->a, "a")},
                                              {"b", serialize_harness(fn->b, "b")}
                                          };
                                      },
                                      [&](const std::shared_ptr<equal_direction8_condition> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "equal_direction8_condition"},
                                              {"description", "Direction8 equality condition"},
                                              {"a", serialize_harness(fn->a, "a")},
                                              {"b", serialize_harness(fn->b, "b")}
                                          };
                                      },
                                      [&](const std::shared_ptr<mousedown_condition> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "mousedown_condition"},
                                              {"description", "Mouse down condition"}
                                          };
                                      },
                                      [&](const std::shared_ptr<mouseover_condition> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "mouseover_condition"},
                                              {"description", "Mouse over condition"}
                                          };
                                      },
                                      [&](const std::shared_ptr<mouseclick_condition> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "mouseclick_condition"},
                                              {"description", "Mouse click condition"}
                                          };
                                      },
                                      [&](const std::shared_ptr<initial_element_condition> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "initial_element_condition"},
                                              {"description", "Initial element condition"}
                                          };
                                      },
                                      [&](const std::shared_ptr<following_element_condition> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "following_element_condition"},
                                              {"description", "Following element condition"}
                                          };
                                      },
                                      [&](const std::shared_ptr<top_level_condition> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "top_level_condition"},
                                              {"description", "Top level condition"}
                                          };
                                      },
                                      [&](const std::shared_ptr<lower_level_condition> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "lower_level_condition"},
                                              {"description", "Lower level condition"}
                                          };
                                      },


                                      [&](const auto &fn) {
                                          // Placeholder for other types
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "unimplemented function"}
                                              // Replace with actual type identification if needed
                                              // Other placeholder fields...
                                          };
                                      }
                                  }, wrapper.my_condition_fn);
                   },
                   [&](const any_gen_fn &wrapper) {
                       std::visit(overloaded{
                                      [&](const std::shared_ptr<identity_gen_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "identity_gen_fn"},
                                              {"description", "General identity function"}
                                          };
                                      },
                                      [&](const std::shared_ptr<orientation_gen_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "orientation_gen_fn"},
                                              {"description", "Orientation generation function"}
                                          };
                                      },
                                      [&](const std::shared_ptr<scale_gen_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "scale_gen_fn"},
                                              {"description", "Scale generation function"}
                                          };
                                      },
                                      [&](const std::shared_ptr<rotation_gen_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "rotation_gen_fn"},
                                              {"description", "Rotation generation function"}
                                          };
                                      },
                                      [&](const std::shared_ptr<position_gen_fn> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "position_gen_fn"},
                                              {"description", "Position generation function"}
                                          };
                                      },
                                      [&](const std::shared_ptr<filter> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "filter"},
                                              {"description", "Filter function"}
                                          };
                                      },
                                      [&](const std::shared_ptr<advect_element> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "advect_element"},
                                              {"description", "Advect element function"}
                                          };
                                      },
                                      [&](const std::shared_ptr<angle_branch> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "angle_branch"},
                                              {"description", "Angle branch function"}
                                          };
                                      },
                                      [&](const std::shared_ptr<curly> &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "curly"},
                                              {"description", "Curly function"}
                                          };
                                      },
                                      [&](const auto &fn) {
                                          j = nlohmann::json{
                                              {"name", wrapper.name},
                                              {"type", "unimplemented gen function"}
                                          };
                                      }
                                  }, wrapper.my_gen_fn);
                   },
                   [&](auto &wrapper) {
                       // Placeholder for other types
                       j = nlohmann::json{
                           {"name", "unimplemented return type"},
                       };
                   }
               }, af);
}

void scene_reader::restore_harness_functions() {
    std::cout << "DEBUG: Restoring harness functions after all functions are loaded" << std::endl;
    
    if (!deferred_harness_json.contains("functions")) {
        std::cout << "DEBUG: No functions to restore" << std::endl;
        return;
    }
    
    // Now restore harness functions for all functions that have them
    for (auto &jfunc: deferred_harness_json["functions"]) {
        std::string func_name;
        if (jfunc.contains("name")) {
            jfunc["name"].get_to(func_name);
        } else {
            continue;
        }
        
        // Find the loaded function and restore its harness functions
        if (s.functions.contains(func_name)) {
            auto& func_variant = s.functions[func_name];
            
            // Handle different function types that have harnesses
            std::visit(overloaded{
                [&](const any_fn<float>& fn) {
                    restore_harness_for_function<float>(jfunc, fn);
                },
                [&](const any_fn<int>& fn) {
                    restore_harness_for_function<int>(jfunc, fn);
                },
                [&](const any_fn<bool>& fn) {
                    restore_harness_for_function<bool>(jfunc, fn);
                },
                [&](const any_fn<std::string>& fn) {
                    restore_harness_for_function<std::string>(jfunc, fn);
                },
                [&](const any_fn<vec2f>& fn) {
                    restore_harness_for_function<vec2f>(jfunc, fn);
                },
                [&](const any_fn<vec2i>& fn) {
                    restore_harness_for_function<vec2i>(jfunc, fn);
                },
                [&](const auto& fn) {
                    // Other types - no harness restoration needed
                }
            }, func_variant);
        }
    }
    
    std::cout << "DEBUG: Harness function restoration completed" << std::endl;
}

template<class T>
void scene_reader::restore_harness_for_function(const json& jfunc, const any_fn<T>& fn) {
    // This is a template method that will be instantiated for each type
    // For now, we'll implement the common cases inline
}

// Template specializations for specific types
template<>
void scene_reader::restore_harness_for_function(const json& jfunc, const any_fn<float>& fn) {
    std::visit(overloaded{
        [&](const std::shared_ptr<slider_float>& slider) {
            if (jfunc.contains("value") && jfunc["value"].is_object() && jfunc["value"].contains("functions")) {
                std::cout << "DEBUG: Restoring harness functions for slider_float '" << slider->label << "'" << std::endl;
                slider->value.functions.clear();
                for (const std::string& func_name : jfunc["value"]["functions"]) {
                    if (s.functions.contains(func_name)) {
                        auto& func_variant = s.functions[func_name];
                        if (std::holds_alternative<any_fn<float>>(func_variant)) {
                            slider->value.add_function(std::get<any_fn<float>>(func_variant));
                            std::cout << "DEBUG: Connected function '" << func_name << "' to slider_float '" << slider->label << "'" << std::endl;
                        }
                    }
                }
            }
        },
        [&](const std::shared_ptr<audio_adder_fn>& audio_fn) {
            // Restore all harness parameters for audio_adder_fn
            auto restore_audio_harness = [&](const std::string& field_name, auto& harness_field) {
                if (jfunc.contains(field_name) && jfunc[field_name].is_object() && jfunc[field_name].contains("functions")) {
                    std::cout << "DEBUG: Restoring " << field_name << " harness for audio_adder_fn" << std::endl;
                    harness_field.functions.clear();
                    for (const std::string& func_name : jfunc[field_name]["functions"]) {
                        if (s.functions.contains(func_name)) {
                            auto& func_variant = s.functions[func_name];
                            if constexpr (std::is_same_v<std::decay_t<decltype(harness_field)>, harness<float>>) {
                                if (std::holds_alternative<any_fn<float>>(func_variant)) {
                                    harness_field.add_function(std::get<any_fn<float>>(func_variant));
                                    std::cout << "DEBUG: Connected function '" << func_name << "' to " << field_name << std::endl;
                                }
                            } else if constexpr (std::is_same_v<std::decay_t<decltype(harness_field)>, harness<std::string>>) {
                                if (std::holds_alternative<any_fn<std::string>>(func_variant)) {
                                    harness_field.add_function(std::get<any_fn<std::string>>(func_variant));
                                    std::cout << "DEBUG: Connected function '" << func_name << "' to " << field_name << std::endl;
                                }
                            }
                        }
                    }
                }
            };
            
            restore_audio_harness("volume_channel", audio_fn->volume_channel);
            restore_audio_harness("volume_weight", audio_fn->volume_weight);
            restore_audio_harness("volume_sensitivity", audio_fn->volume_sensitivity);
            restore_audio_harness("bass_channel", audio_fn->bass_channel);
            restore_audio_harness("bass_weight", audio_fn->bass_weight);
            restore_audio_harness("bass_sensitivity", audio_fn->bass_sensitivity);
            restore_audio_harness("mid_channel", audio_fn->mid_channel);
            restore_audio_harness("mid_weight", audio_fn->mid_weight);
            restore_audio_harness("mid_sensitivity", audio_fn->mid_sensitivity);
            restore_audio_harness("high_channel", audio_fn->high_channel);
            restore_audio_harness("high_weight", audio_fn->high_weight);
            restore_audio_harness("high_sensitivity", audio_fn->high_sensitivity);
            restore_audio_harness("offset", audio_fn->offset);
            restore_audio_harness("global_sensitivity", audio_fn->global_sensitivity);
        },
        [&](const std::shared_ptr<integrator_float>& integrator) {
            // Restore integrator runtime state
            if (jfunc.contains("delta") && jfunc["delta"].is_object() && jfunc["delta"].contains("functions")) {
                std::cout << "DEBUG: Restoring delta harness for integrator_float" << std::endl;
                integrator->delta.functions.clear();
                for (const std::string& func_name : jfunc["delta"]["functions"]) {
                    if (s.functions.contains(func_name)) {
                        auto& func_variant = s.functions[func_name];
                        if (std::holds_alternative<any_fn<float>>(func_variant)) {
                            integrator->delta.add_function(std::get<any_fn<float>>(func_variant));
                            std::cout << "DEBUG: Connected function '" << func_name << "' to integrator delta" << std::endl;
                        }
                    }
                }
            }
            // Runtime state (val) is already restored during function loading, just log it
            std::cout << "DEBUG: integrator_float runtime val=" << integrator->val << std::endl;
        },
        [&](const std::shared_ptr<generator_float>& generator) {
            // Restore all harness parameters for generator_float
            auto restore_gen_harness = [&](const std::string& field_name, auto& harness_field) {
                if (jfunc.contains(field_name) && jfunc[field_name].is_object() && jfunc[field_name].contains("functions")) {
                    std::cout << "DEBUG: Restoring " << field_name << " harness for generator_float" << std::endl;
                    harness_field.functions.clear();
                    for (const std::string& func_name : jfunc[field_name]["functions"]) {
                        if (s.functions.contains(func_name)) {
                            auto& func_variant = s.functions[func_name];
                            if constexpr (std::is_same_v<std::decay_t<decltype(harness_field)>, harness<float>>) {
                                if (std::holds_alternative<any_fn<float>>(func_variant)) {
                                    harness_field.add_function(std::get<any_fn<float>>(func_variant));
                                    std::cout << "DEBUG: Connected function '" << func_name << "' to " << field_name << std::endl;
                                }
                            } else if constexpr (std::is_same_v<std::decay_t<decltype(harness_field)>, harness<bool>>) {
                                if (std::holds_alternative<any_fn<bool>>(func_variant)) {
                                    harness_field.add_function(std::get<any_fn<bool>>(func_variant));
                                    std::cout << "DEBUG: Connected function '" << func_name << "' to " << field_name << std::endl;
                                }
                            }
                        }
                    }
                }
            };
            
            restore_gen_harness("p", generator->p);
            restore_gen_harness("a", generator->a);
            restore_gen_harness("b", generator->b);
            restore_gen_harness("enabled", generator->enabled);
        },
        [&](const std::shared_ptr<tweaker_float>& tweaker) {
            // Restore harness parameters for tweaker_float
            auto restore_tweaker_harness = [&](const std::string& field_name, auto& harness_field) {
                if (jfunc.contains(field_name) && jfunc[field_name].is_object() && jfunc[field_name].contains("functions")) {
                    std::cout << "DEBUG: Restoring " << field_name << " harness for tweaker_float" << std::endl;
                    harness_field.functions.clear();
                    for (const std::string& func_name : jfunc[field_name]["functions"]) {
                        if (s.functions.contains(func_name)) {
                            auto& func_variant = s.functions[func_name];
                            if constexpr (std::is_same_v<std::decay_t<decltype(harness_field)>, harness<float>>) {
                                if (std::holds_alternative<any_fn<float>>(func_variant)) {
                                    harness_field.add_function(std::get<any_fn<float>>(func_variant));
                                    std::cout << "DEBUG: Connected function '" << func_name << "' to " << field_name << std::endl;
                                }
                            } else if constexpr (std::is_same_v<std::decay_t<decltype(harness_field)>, harness<bool>>) {
                                if (std::holds_alternative<any_fn<bool>>(func_variant)) {
                                    harness_field.add_function(std::get<any_fn<bool>>(func_variant));
                                    std::cout << "DEBUG: Connected function '" << func_name << "' to " << field_name << std::endl;
                                }
                            }
                        }
                    }
                }
            };
            
            restore_tweaker_harness("p", tweaker->p);
            restore_tweaker_harness("amount", tweaker->amount);
            restore_tweaker_harness("enabled", tweaker->enabled);
        },
        [&](const auto& other_fn) {
            // Other float function types - no harness restoration needed
        }
    }, fn.any_fn_ptr);
}

template<>
void scene_reader::restore_harness_for_function(const json& jfunc, const any_fn<int>& fn) {
    std::visit(overloaded{
        [&](const std::shared_ptr<slider_int>& slider) {
            if (jfunc.contains("value") && jfunc["value"].is_object() && jfunc["value"].contains("functions")) {
                std::cout << "DEBUG: Restoring harness functions for slider_int '" << slider->label << "'" << std::endl;
                slider->value.functions.clear();
                for (const std::string& func_name : jfunc["value"]["functions"]) {
                    if (s.functions.contains(func_name)) {
                        auto& func_variant = s.functions[func_name];
                        if (std::holds_alternative<any_fn<int>>(func_variant)) {
                            slider->value.add_function(std::get<any_fn<int>>(func_variant));
                            std::cout << "DEBUG: Connected function '" << func_name << "' to slider_int '" << slider->label << "'" << std::endl;
                        }
                    }
                }
            }
        },
        [&](const std::shared_ptr<menu_int>& menu) {
            if (jfunc.contains("choice") && jfunc["choice"].is_object() && jfunc["choice"].contains("functions")) {
                std::cout << "DEBUG: Restoring harness functions for menu_int '" << menu->label << "'" << std::endl;
                menu->choice.functions.clear();
                for (const std::string& func_name : jfunc["choice"]["functions"]) {
                    if (s.functions.contains(func_name)) {
                        auto& func_variant = s.functions[func_name];
                        if (std::holds_alternative<any_fn<int>>(func_variant)) {
                            menu->choice.add_function(std::get<any_fn<int>>(func_variant));
                            std::cout << "DEBUG: Connected function '" << func_name << "' to menu_int '" << menu->label << "'" << std::endl;
                        }
                    }
                }
            }
        },
        [&](const std::shared_ptr<generator_int>& generator) {
            // Restore all harness parameters for generator_int
            auto restore_gen_harness = [&](const std::string& field_name, auto& harness_field) {
                if (jfunc.contains(field_name) && jfunc[field_name].is_object() && jfunc[field_name].contains("functions")) {
                    std::cout << "DEBUG: Restoring " << field_name << " harness for generator_int" << std::endl;
                    harness_field.functions.clear();
                    for (const std::string& func_name : jfunc[field_name]["functions"]) {
                        if (s.functions.contains(func_name)) {
                            auto& func_variant = s.functions[func_name];
                            if constexpr (std::is_same_v<std::decay_t<decltype(harness_field)>, harness<float>>) {
                                if (std::holds_alternative<any_fn<float>>(func_variant)) {
                                    harness_field.add_function(std::get<any_fn<float>>(func_variant));
                                    std::cout << "DEBUG: Connected function '" << func_name << "' to " << field_name << std::endl;
                                }
                            } else if constexpr (std::is_same_v<std::decay_t<decltype(harness_field)>, harness<int>>) {
                                if (std::holds_alternative<any_fn<int>>(func_variant)) {
                                    harness_field.add_function(std::get<any_fn<int>>(func_variant));
                                    std::cout << "DEBUG: Connected function '" << func_name << "' to " << field_name << std::endl;
                                }
                            } else if constexpr (std::is_same_v<std::decay_t<decltype(harness_field)>, harness<bool>>) {
                                if (std::holds_alternative<any_fn<bool>>(func_variant)) {
                                    harness_field.add_function(std::get<any_fn<bool>>(func_variant));
                                    std::cout << "DEBUG: Connected function '" << func_name << "' to " << field_name << std::endl;
                                }
                            }
                        }
                    }
                }
            };
            
            restore_gen_harness("p", generator->p);
            restore_gen_harness("a", generator->a);
            restore_gen_harness("b", generator->b);
            restore_gen_harness("enabled", generator->enabled);
        },
        [&](const auto& other_fn) {
            // Other int function types - no harness restoration needed
        }
    }, fn.any_fn_ptr);
}

template<>
void scene_reader::restore_harness_for_function(const json& jfunc, const any_fn<bool>& fn) {
    std::visit(overloaded{
        [&](const std::shared_ptr<switch_fn>& switch_ptr) {
            if (jfunc.contains("value") && jfunc["value"].is_object() && jfunc["value"].contains("functions")) {
                std::cout << "DEBUG: Restoring harness functions for switch_fn '" << switch_ptr->label << "'" << std::endl;
                switch_ptr->value.functions.clear();
                for (const std::string& func_name : jfunc["value"]["functions"]) {
                    if (s.functions.contains(func_name)) {
                        auto& func_variant = s.functions[func_name];
                        if (std::holds_alternative<any_fn<bool>>(func_variant)) {
                            switch_ptr->value.add_function(std::get<any_fn<bool>>(func_variant));
                            std::cout << "DEBUG: Connected function '" << func_name << "' to switch_fn '" << switch_ptr->label << "'" << std::endl;
                        }
                    }
                }
            }
        },
        [&](const std::shared_ptr<random_toggle>& toggle) {
            // Restore harness parameters for random_toggle
            auto restore_toggle_harness = [&](const std::string& field_name, auto& harness_field) {
                if (jfunc.contains(field_name) && jfunc[field_name].is_object() && jfunc[field_name].contains("functions")) {
                    std::cout << "DEBUG: Restoring " << field_name << " harness for random_toggle" << std::endl;
                    harness_field.functions.clear();
                    for (const std::string& func_name : jfunc[field_name]["functions"]) {
                        if (s.functions.contains(func_name)) {
                            auto& func_variant = s.functions[func_name];
                            if constexpr (std::is_same_v<std::decay_t<decltype(harness_field)>, harness<float>>) {
                                if (std::holds_alternative<any_fn<float>>(func_variant)) {
                                    harness_field.add_function(std::get<any_fn<float>>(func_variant));
                                    std::cout << "DEBUG: Connected function '" << func_name << "' to " << field_name << std::endl;
                                }
                            } else if constexpr (std::is_same_v<std::decay_t<decltype(harness_field)>, harness<bool>>) {
                                if (std::holds_alternative<any_fn<bool>>(func_variant)) {
                                    harness_field.add_function(std::get<any_fn<bool>>(func_variant));
                                    std::cout << "DEBUG: Connected function '" << func_name << "' to " << field_name << std::endl;
                                }
                            }
                        }
                    }
                }
            };
            
            restore_toggle_harness("enabled", toggle->enabled);
            restore_toggle_harness("p", toggle->p);
        },
        [&](const auto& other_fn) {
            // Other bool function types - no harness restoration needed yet
        }
    }, fn.any_fn_ptr);
}

template<>
void scene_reader::restore_harness_for_function(const json& jfunc, const any_fn<std::string>& fn) {
    std::visit(overloaded{
        [&](const std::shared_ptr<menu_string>& menu) {
            if (jfunc.contains("choice") && jfunc["choice"].is_object() && jfunc["choice"].contains("functions")) {
                std::cout << "DEBUG: Restoring harness functions for menu_string '" << menu->label << "'" << std::endl;
                menu->choice.functions.clear();
                for (const std::string& func_name : jfunc["choice"]["functions"]) {
                    if (s.functions.contains(func_name)) {
                        auto& func_variant = s.functions[func_name];
                        if (std::holds_alternative<any_fn<int>>(func_variant)) {
                            menu->choice.add_function(std::get<any_fn<int>>(func_variant));
                            std::cout << "DEBUG: Connected function '" << func_name << "' to menu_string '" << menu->label << "'" << std::endl;
                        }
                    }
                }
            }
        },
        [&](const auto& other_fn) {
            // Other string function types - no harness restoration needed
        }
    }, fn.any_fn_ptr);
}

template<>
void scene_reader::restore_harness_for_function(const json& jfunc, const any_fn<vec2f>& fn) {
    std::visit(overloaded{
        [&](const std::shared_ptr<adder_vec2f>& adder) {
            if (jfunc.contains("r") && jfunc["r"].is_object() && jfunc["r"].contains("functions")) {
                std::cout << "DEBUG: Restoring harness functions for adder_vec2f" << std::endl;
                adder->r.functions.clear();
                for (const std::string& func_name : jfunc["r"]["functions"]) {
                    if (s.functions.contains(func_name)) {
                        auto& func_variant = s.functions[func_name];
                        if (std::holds_alternative<any_fn<vec2f>>(func_variant)) {
                            adder->r.add_function(std::get<any_fn<vec2f>>(func_variant));
                            std::cout << "DEBUG: Connected function '" << func_name << "' to adder_vec2f" << std::endl;
                        }
                    }
                }
            }
        },
        [&](const auto& other_fn) {
            // Other vec2f function types - no harness restoration needed
        }
    }, fn.any_fn_ptr);
}

template<>
void scene_reader::restore_harness_for_function(const json& jfunc, const any_fn<vec2i>& fn) {
    std::visit(overloaded{
        [&](const std::shared_ptr<buffer_dim_fn>& buf_dim) {
            if (jfunc.contains("buf_name") && jfunc["buf_name"].is_object() && jfunc["buf_name"].contains("functions")) {
                std::cout << "DEBUG: Restoring buf_name harness for buffer_dim_fn" << std::endl;
                buf_dim->buf_name.functions.clear();
                for (const std::string& func_name : jfunc["buf_name"]["functions"]) {
                    if (s.functions.contains(func_name)) {
                        auto& func_variant = s.functions[func_name];
                        if (std::holds_alternative<any_fn<std::string>>(func_variant)) {
                            buf_dim->buf_name.add_function(std::get<any_fn<std::string>>(func_variant));
                            std::cout << "DEBUG: Connected function '" << func_name << "' to buffer_dim_fn buf_name" << std::endl;
                        }
                    }
                }
            }
        },
        [&](const auto& other_fn) {
            // Other vec2i function types - no harness restoration needed
        }
    }, fn.any_fn_ptr);
}

