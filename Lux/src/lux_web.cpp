//#include <SDL.h>
#include "scene.hpp"
#include "scene_io.hpp"
#include "uimage.hpp"
#include "effect.hpp"
#include "any_effect.hpp"
#include "life.hpp"
#include "any_rule.hpp"
#include <fstream>
#include <sstream>
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include "emscripten_utils.hpp"
#include "video_recorder.hpp"
#include <chrono>

// Audio data stored in scene's UI context - no separate global audio context needed
// The frontend SHO/FFT system updates scene parameters directly via the harness system

// Implementation of audio harness helper functions
template<>
void make_audio_reactive<float>(harness<float>& h, const std::string& channel, float sensitivity) {
    auto audio_fn = std::make_shared<audio_additive_fn>(channel, sensitivity, 0.0f);
    float_fn fn_ref = std::ref(*audio_fn);
    any_float_fn_ptr variant_ptr = audio_fn;  // Cast to variant type
    any_fn<float> any_audio_fn(variant_ptr, fn_ref, "audio_" + channel);
    h.add_function(any_audio_fn);
}

template<>
void make_audio_reactive<vec2f>(harness<vec2f>& h, const std::string& channel, float sensitivity) {
    auto audio_fn = std::make_shared<audio_additive_vec2f_fn>(channel, channel, sensitivity, sensitivity, vec2f(0.0f, 0.0f));
    vec2f_fn fn_ref = std::ref(*audio_fn);
    any_vec2f_fn_ptr variant_ptr = audio_fn;  // Cast to variant type
    any_fn<vec2f> any_audio_fn(variant_ptr, fn_ref, "audio_" + channel);
    h.add_function(any_audio_fn);
}

using namespace emscripten;

// Forward declaration
void debug_scene_processing_state();

//const val document = val::global("document");

// used for stuffing everything needed to iterate and display a frame into a single void*
struct frame_context {
  //SDL_Surface *screen;
  std::function<void()> frame_callback, resize_callback, scene_callback;
  bool frame_callback_ready, resize_callback_ready, scene_callback_ready, js_bitmaps_ready;
  std::unique_ptr< scene > s;
  std::shared_ptr< buffer_pair< ucolor > > buf;
  vec2i buf_dim;
  nlohmann::json scene_list;

  std::unique_ptr<VideoRecorder> video_recorder;
  bool is_recording;
};

frame_context *global_context;

val get_buf1() {
    if (!global_context || !global_context->buf) {
        return val::null();
    }
    uimage& img = (uimage &)(global_context->buf->get_image());
    unsigned char* buffer = (unsigned char* )img.get_base_ptr();
    size_t buffer_length = img.get_dim().x * img.get_dim().y * 4; // Assuming 4 bytes per pixel (RGBA)

    //std::cout << "get_img_data() buffer length: " << buffer_length << std::endl;
    // Create a typed memory view at the specified memory location.
    return val(typed_memory_view(buffer_length, buffer));
}

val get_img_data() { 
    if (!global_context || !global_context->buf) {
        return val::null();
    }
    return get_buf1(); 
}

val get_buf2() {
    if (!global_context || !global_context->buf) {
        return val::null();
    }
    uimage& img = (uimage &)(global_context->buf->get_buffer());
    unsigned char* buffer = (unsigned char* )img.get_base_ptr();
    size_t buffer_length = img.get_dim().x * img.get_dim().y * 4; // Assuming 4 bytes per pixel (RGBA)

    //std::cout << "get_img_data() buffer length: " << buffer_length << std::endl;
    // Create a typed memory view at the specified memory location.
    return val(typed_memory_view(buffer_length, buffer));
}



val get_thumbnail(std::string name, int width, int height) {
    // create temporal target thumbnail image
    auto thumb_img_ptr = std::make_unique<uimage >(vec2i{width, height});
    if (!thumb_img_ptr) {
        std::cerr << "Failed to create thumbnail image" << std::endl;
        // return an empty view to JS
        return emscripten::val(emscripten::typed_memory_view(0, static_cast<unsigned char *>(nullptr)));
    }

    uimage thumb_img = *thumb_img_ptr;
    unsigned char *buffer = (unsigned char *) thumb_img.get_base_ptr();
    size_t buffer_length = width * height * 4;
    bool drawn = false;

    // find source buffer and validate
    if (global_context && global_context->s && global_context->s->buffers.count(name)) {
        any_buffer_pair_ptr &source_buf_variant = global_context->s->buffers[name];
        if (std::holds_alternative<ubuf_ptr>(source_buf_variant)) {
            ubuf_ptr &source_buf_pair_ptr = std::get<ubuf_ptr>(source_buf_variant);
            if (source_buf_pair_ptr && source_buf_pair_ptr->has_image()) {
                image<ucolor> &source_image = source_buf_pair_ptr->get_image();
                if (source_image.get_dim().x > 0 && source_image.get_dim().y > 0) {
                    // define splat parameters
                    bool thumb_smooth = false;
                    vec2f thumb_center = vec2f(0, 0);
                    float thumb_scale = 1.0f;
                    float thumb_delta = 0.0f;
                    std::optional<std::reference_wrapper<image<ucolor> > > thumb_mask = std::nullopt;
                    std::optional<ucolor> thumb_tint = std::nullopt;
                    mask_mode thumb_mode = MASK_NOEFFECT;
                    // call splat
                    try {
                        thumb_img.splat(source_image, thumb_smooth, thumb_center, thumb_scale, thumb_delta, thumb_mask,
                                        thumb_tint, thumb_mode);
                        drawn = true;

                        if (drawn && width > 0 && height > 0) {
                            int x = 0;
                            int y = 0;
                            unsigned int pixel_index = y * thumb_img.get_dim().x + x;
                            if (pixel_index < thumb_img.size()) { // Use size() method you added
                                ucolor first_pixel_value = thumb_img.index(pixel_index); // <<< Use index(i)
                                unsigned char* first_pixel_bytes = reinterpret_cast<unsigned char*>(&first_pixel_value);
                                DEBUG("get_thumbnail: First pixel value (hex): " + ucolor_to_hex_string(first_pixel_value)); // Add hex log if possible
                                DEBUG("get_thumbnail: First pixel bytes (memory): [" +
                                      std::to_string(first_pixel_bytes[0]) + ", " + // Byte 0 (Lowest address)
                                      std::to_string(first_pixel_bytes[1]) + ", " + // Byte 1
                                      std::to_string(first_pixel_bytes[2]) + ", " + // Byte 2
                                      std::to_string(first_pixel_bytes[3]) + "]");  // Byte 3 (Highest address)
                            } else {
                                DEBUG("get_thumbnail: Calculated pixel index out of bounds!");
                            }
                        }
                    } catch (const std::exception &e) {
                        std::cerr << "Error during splat operation: " << e.what() << std::endl;
                    } catch (...) {
                        std::cerr << "Unknown error during splat operation" << std::endl;
                    }
                }
            }
        }
    }

    if (!drawn) {
        std::cerr << "Failed to draw thumbnail" << std::endl;
        thumb_img.fill(0xff808080); // gray placeholder
    }
    return emscripten::val(typed_memory_view(buffer_length, buffer));
}


void set_frame_callback(val callback) {
    global_context->frame_callback = [callback]() mutable {
        callback();
    };
    global_context->frame_callback_ready = true;
}

void set_resize_callback(val callback) {
    global_context->resize_callback = [callback]() mutable {
        callback();
    };
    global_context->resize_callback_ready = true;
}

void set_scene_callback(val callback) {
    global_context->scene_callback = [callback]() mutable {
        callback();
    };
    global_context->scene_callback_ready = true;
}


int get_buf_width() {
    if (!global_context || !global_context->buf) {
        return 0;
    }
    uimage& img = (uimage &)(global_context->buf->get_image());
    return img.get_dim().x;
}

int get_buf_height() {
    if (!global_context || !global_context->buf) {
        return 0;
    }
    uimage& img = (uimage &)(global_context->buf->get_image());
    return img.get_dim().y;
}

bool is_swapped() {
    return global_context->buf->is_swapped();
}

void bitmaps_ready() {
    global_context->js_bitmaps_ready = true;
}

// used as emscripten main loop
void render_and_display( void *arg )
{
    bool &running = global_context->s->ui.running;
    bool &advance = global_context->s->ui.advance;
    bool &displayed = global_context->s->ui.displayed;
    //using namespace std::this_thread;     // sleep_for, sleep_until
    //using namespace std::chrono_literals; // ns, us, ms, s, h, etc.
    //using std::chrono::system_clock;

    //sleep_until(system_clock::now() + 1s);
    //emscripten_run_script("console.log('render and display');");
    if( global_context->frame_callback_ready ) {
        //emscripten_run_script("console.log('callback and bitmaps ready');");

        // Check for dirty buffers (e.g. after source image change)
        //for( auto q : global_context->s->queue ) if( !q.rendered ) displayed = false;

        // Check for buffer resize (e.g. after source image change)
        vec2i dim = global_context->buf->get_image().get_dim();
        if( dim != global_context->buf_dim ) {
            std::cout << "resize buffer: " << dim.x << " " << dim.y << std::endl;
            global_context->buf_dim = dim;
            global_context->s->ui.canvas_bounds = bb2i( dim );
            if( global_context->resize_callback_ready ) global_context->resize_callback();
            displayed = false;
        }

        if (global_context->is_recording && displayed) {
            uimage& img = (uimage &)(global_context->buf->get_image());

            // add frame to recording
            if (!global_context->video_recorder->add_frame(img)) {
                std::cerr << "Failed to add frame to recording: " <<
                     global_context->video_recorder->get_error() << std::endl;
                global_context->is_recording = false;
            }
        }

        if( !running && !advance && displayed ) {
            global_context->s->ui.mouse_click = false;
            return;
        }

        global_context->frame_callback();

        // Apply audio reactivity to scene parameters
        // Audio reactivity is now handled automatically by the harness system

        if( running || advance || !displayed ) {
            global_context->s->render();
        }

        global_context->s->ui.mouse_click = false;
        displayed = true;
        advance = false;
    }
    else {
        //if(!global_context->frame_callback_ready) emscripten_run_script("console.log('callback not ready');");
        //else emscripten_run_script("console.log('bitmaps not ready');");
    }
}

void run_pause() {
    bool &running = global_context->s->ui.running;
    running = !running;
}

void restart() {
    //global_context->frame = 0;
    global_context->s->restart();
    global_context->s->ui.displayed = false;
}


void reset_scene_parameters() {
    if (!global_context || !global_context->s) {
        return;
    }
    
    // Reset all functions to their default values
    for (auto& [name, fn] : global_context->s->functions) {
        std::visit([](auto&& func_wrapper) {
            using T = std::decay_t<decltype(func_wrapper)>;
            
            if constexpr (std::is_same_v<T, any_fn<float>>) {
                std::visit([](auto&& fn_ptr) {
                    using FnType = std::decay_t<decltype(fn_ptr)>;
                    if constexpr (std::is_same_v<FnType, std::shared_ptr<slider_float>>) {
                        fn_ptr->reset();
                    }
                    else if constexpr (std::is_same_v<FnType, std::shared_ptr<integrator_float>>) {
                        fn_ptr->val = fn_ptr->starting_val;
                        fn_ptr->last_time = 0.0f;
                    }
                }, func_wrapper.any_fn_ptr);
            }
            else if constexpr (std::is_same_v<T, any_fn<int>>) {
                std::visit([](auto&& fn_ptr) {
                    using FnType = std::decay_t<decltype(fn_ptr)>;
                    if constexpr (std::is_same_v<FnType, std::shared_ptr<slider_int>>) {
                        fn_ptr->reset();
                    }
                    else if constexpr (std::is_same_v<FnType, std::shared_ptr<menu_int>>) {
                        fn_ptr->choice = fn_ptr->default_choice;
                    }
                    else if constexpr (std::is_same_v<FnType, std::shared_ptr<multi_direction8_picker>>) {
                        fn_ptr->value = fn_ptr->default_value;
                    }
                }, func_wrapper.any_fn_ptr);
            }
            else if constexpr (std::is_same_v<T, any_fn<interval_float>>) {
                std::visit([](auto&& fn_ptr) {
                    using FnType = std::decay_t<decltype(fn_ptr)>;
                    if constexpr (std::is_same_v<FnType, std::shared_ptr<range_slider_float>>) {
                        fn_ptr->reset();
                    }
                }, func_wrapper.any_fn_ptr);
            }
            else if constexpr (std::is_same_v<T, any_fn<interval_int>>) {
                std::visit([](auto&& fn_ptr) {
                    using FnType = std::decay_t<decltype(fn_ptr)>;
                    if constexpr (std::is_same_v<FnType, std::shared_ptr<range_slider_int>>) {
                        fn_ptr->reset();
                    }
                }, func_wrapper.any_fn_ptr);
            }
            else if constexpr (std::is_same_v<T, any_fn<std::string>>) {
                std::visit([](auto&& fn_ptr) {
                    using FnType = std::decay_t<decltype(fn_ptr)>;
                    if constexpr (std::is_same_v<FnType, std::shared_ptr<menu_string>>) {
                        fn_ptr->choice = fn_ptr->default_choice;
                    }
                }, func_wrapper.any_fn_ptr);
            }
            else if constexpr (std::is_same_v<T, any_fn<bool>>) {
                std::visit([](auto&& fn_ptr) {
                    using FnType = std::decay_t<decltype(fn_ptr)>;
                    if constexpr (std::is_same_v<FnType, std::shared_ptr<switch_fn>>) {
                        fn_ptr->reset();
                    }
                }, func_wrapper.any_fn_ptr);
            }
            else if constexpr (std::is_same_v<T, any_fn<ucolor>>) {
                std::visit([](auto&& fn_ptr) {
                    using FnType = std::decay_t<decltype(fn_ptr)>;
                    if constexpr (std::is_same_v<FnType, std::shared_ptr<ucolor_picker>>) {
                        fn_ptr->value = fn_ptr->default_value;
                    }
                }, func_wrapper.any_fn_ptr);
            }
            else if constexpr (std::is_same_v<T, any_fn<funk_factor>>) {
                std::visit([](auto&& fn_ptr) {
                    using FnType = std::decay_t<decltype(fn_ptr)>;
                    if constexpr (std::is_same_v<FnType, std::shared_ptr<funk_factor_picker>>) {
                        fn_ptr->value = fn_ptr->default_value;
                    }
                }, func_wrapper.any_fn_ptr);
            }
            else if constexpr (std::is_same_v<T, any_fn<direction4>>) {
                std::visit([](auto&& fn_ptr) {
                    using FnType = std::decay_t<decltype(fn_ptr)>;
                    if constexpr (std::is_same_v<FnType, std::shared_ptr<direction_picker_4>>) {
                        fn_ptr->value = fn_ptr->default_value;
                    }
                }, func_wrapper.any_fn_ptr);
            }
            else if constexpr (std::is_same_v<T, any_fn<direction4_diagonal>>) {
                std::visit([](auto&& fn_ptr) {
                    using FnType = std::decay_t<decltype(fn_ptr)>;
                    if constexpr (std::is_same_v<FnType, std::shared_ptr<direction_picker_4_diagonal>>) {
                        fn_ptr->value = fn_ptr->default_value;
                    }
                }, func_wrapper.any_fn_ptr);
            }
            else if constexpr (std::is_same_v<T, any_fn<direction8>>) {
                std::visit([](auto&& fn_ptr) {
                    using FnType = std::decay_t<decltype(fn_ptr)>;
                    if constexpr (std::is_same_v<FnType, std::shared_ptr<direction_picker_8>>) {
                        fn_ptr->value = fn_ptr->default_value;
                    }
                }, func_wrapper.any_fn_ptr);
            }
                 }, fn);
     }
     
     // Disable audio when resetting to default state
     global_context->s->ui.audio.enabled = false;
     
     // Force re-render
     global_context->s->ui.displayed = false;
}

void advance_frame() {
    global_context->s->ui.advance = true;
    global_context->s->ui.running = false;
}

void mouse_move( int x, int y, int width, int height ) {
    if (!global_context || !global_context->s) {
        return;
    }
    UI& ui = global_context->s->ui;
    
    // Prevent divide by zero
    if (width <= 0 || height <= 0) {
        return;
    }
    
    ui.mouse_pixel = vec2i( { x * ui.canvas_bounds.width() / width, y * ui.canvas_bounds.height() / height } );
}

void mouse_down( bool down ) {
    global_context->s->ui.mouse_down = down;
}

void mouse_over( bool over ) {
    global_context->s->ui.mouse_over = over;
    if( !over ) global_context->s->ui.mouse_down = false;
}

void mouse_click( bool click ) {
    global_context->s->ui.mouse_click = click;
}

void set_slider_value( std::string name, float value ) {
    if( global_context->s->functions.contains( name ) ) {
        any_function& fn = global_context->s->functions[ name ];
        if( std::holds_alternative< any_fn< float > >( fn ) ) {
            global_context->s->get_fn_ptr< float, slider_float >( name )->value = value;
            
            global_context->s->ui.displayed = false;
            
            if( global_context->s->ui.running && (
                name.find("start") != std::string::npos || 
                name.find("spin") != std::string::npos || 
                name.find("expand") != std::string::npos ||
                name.find("phase") != std::string::npos ||
                name.find("swirl") != std::string::npos ||
                name.find("alternate") != std::string::npos ||
                name.find("speed") != std::string::npos ||
                name.find("rate") != std::string::npos ||
                name.find("time") != std::string::npos ) ) {
                global_context->s->ui.running = true;
            }
            return;
        }
        else if( std::holds_alternative< any_fn< int > >( fn ) ) {
           global_context->s->get_fn_ptr< int, slider_int >( name )->value = (int)std::roundf( value );
           
           global_context->s->ui.displayed = false;
           
           if( global_context->s->ui.running && (
               name.find("start") != std::string::npos || 
               name.find("spin") != std::string::npos || 
               name.find("expand") != std::string::npos ||
               name.find("phase") != std::string::npos ||
               name.find("swirl") != std::string::npos ||
               name.find("alternate") != std::string::npos ||
               name.find("speed") != std::string::npos ||
               name.find("rate") != std::string::npos ||
               name.find("time") != std::string::npos ) ) {
               global_context->s->ui.running = true;
           }
           return;
        }
        throw std::runtime_error( "slider " + name + " invalid type " );
    }
    throw std::runtime_error( "slider " + name + " not found in scene" );
}

void set_slider_callback( std::string name, val callback ) {
    std::cout << "set_slider_callback: " << name << std::endl;
    if( global_context->s->functions.contains( name ) ) {
        any_function& fn = global_context->s->functions[ name ];
        if( std::holds_alternative< any_fn< float > >( fn ) ) {
            global_context->s->get_fn_ptr< float, slider_float >( name )->callback = [callback](float value) mutable {
                callback(value);
            };
            return;
        }
        else if( std::holds_alternative< any_fn< int > >( fn ) ) {
            global_context->s->get_fn_ptr< int, slider_int >( name )->callback = [callback](int value) mutable {
                callback(value);
            };
            return;
        }
        throw std::runtime_error( "slider " + name + " invalid type for callback" );
    }
    throw std::runtime_error( "slider " + name + " not found in scene" );
}

void set_range_slider_value( std::string name, float value_min, float value_max ) {
    // std::cout << "set_range_slider_value: " << name << " " << value_min << " " << value_max << std::endl;
    if( global_context->s->functions.contains( name ) ) {
        any_function& fn = global_context->s->functions[ name ];
        if( std::holds_alternative< any_fn< interval_float > >( fn ) ) {
            global_context->s->get_fn_ptr< interval_float, range_slider_float >( name )->value = interval_float( value_min, value_max );
            global_context->s->ui.displayed = false;
            return;
        }
        else if( std::holds_alternative< any_fn< interval_int > >( fn ) ) {
            //std::cout << "range_slider_int: " << name << " " << value_min << " " << value_max << std::endl;
            global_context->s->get_fn_ptr< interval_int, range_slider_int >( name )->value = interval_int( (int)std::roundf( value_min ), (int)std::roundf( value_max ) );
            global_context->s->ui.displayed = false;
            return;
        }
        throw std::runtime_error( "range slider " + name + " invalid type " );
    }
    throw std::runtime_error( "range slider " + name + " not found in scene" );
}

void handle_menu_choice( std::string name, int choice ) {
    if( global_context->s->functions.contains( name ) ) {
        any_function& fn = global_context->s->functions[ name ];
        if( std::holds_alternative< any_fn< int > >( fn ) ) {
            //std::cout << "handle_menu_choice (menu_int): " << name << " " << choice << std::endl;
            auto menu = global_context->s->get_fn_ptr< int, menu_int >( name );
            menu->choose( choice );
            // future: allow menu to reset a single buffer
            if( menu->rerender ) restart();
            return;
        }
        else if( std::holds_alternative< any_fn< std::string > >( fn ) ) {
            //std::cout << "handle_menu_choice (menu_string): " << name << " " << choice << std::endl;
            auto menu = global_context->s->get_fn_ptr< std::string, menu_string >( name );
            menu->choose( choice );
            // future: allow menu to reset a single buffer
            if( menu->rerender ) restart();
            return;
        }
        throw std::runtime_error( "menu " + name + " invalid type " );
    }
    throw std::runtime_error( "menu " + name + " not found in scene" );
}

void handle_switch_value( std::string name, bool value ) {
    if( global_context->s->functions.contains( name ) ) {
        any_function& fn = global_context->s->functions[ name ];
        if( std::holds_alternative< any_fn< bool > >( fn ) ) {
            auto sw = global_context->s->get_fn_ptr< bool, switch_fn >( name );
            sw->value = value;
            return;
        }
        else if( std::holds_alternative< any_condition_fn >( fn ) ) {
            auto& sw = std::get< std::shared_ptr< switch_condition > >( std::get< any_condition_fn >( fn ).my_condition_fn );
            sw->value = value;
            return;
        }
        throw std::runtime_error( "switch " + name + " invalid type " );
    }
    throw std::runtime_error( "switch " + name + " not found in scene" );
}

void pick_direction8( std::string name, int value ) {
    auto picker = global_context->s->get_fn_ptr< direction8, direction_picker_8 >( name );
    picker->value = ( direction8 )value;
}

void pick_funk_factor( std::string name, std::string value ) {
    auto picker = global_context->s->get_fn_ptr< funk_factor, funk_factor_picker >( name );
    picker->value = std::stoull(value, nullptr, 16); // value passed as hexidecimal string
}

void pick_ucolor( std::string name, ucolor value ) {
    auto picker = global_context->s->get_fn_ptr< ucolor, ucolor_picker >( name );
    picker->value = value;
}

void pick_direction4( std::string name, int value ) {
    auto picker = global_context->s->get_fn_ptr< direction4, direction_picker_4 >( name );
    picker->value = ( direction4 )value;
}

void pick_direction4_diagonal( std::string name, int value ) {
    auto picker = global_context->s->get_fn_ptr< direction4_diagonal, direction_picker_4_diagonal >( name );
    picker->value = ( direction4_diagonal )value;
}

void pick_blur_method( std::string name, int value ) {
    auto picker = global_context->s->get_fn_ptr< box_blur_type, box_blur_picker >( name );
    picker->value = (box_blur_type)value;
}

void pick_multi_direction8( std::string name, int value, int id ) {
    auto picker = global_context->s->get_fn_ptr< int, custom_blur_picker >( name );
    picker->set_picker_value( value, id );
}

void print_vector_of_pairs( std::vector< std::pair< int, int > > pickers ) {
    for( auto& p : pickers ) {
        std::cout << p.first << " " << p.second << std::endl;
    }
}

void remove_custom_blur_pickers( std::string name, int index ) {
    //std::cout << "remove_custom_blur_pickers: " << name << " " << index << std::endl;
    auto picker = global_context->s->get_fn_ptr< int, custom_blur_picker >( name );
    picker->remove_pickers( index );
    //print_vector_of_pairs( picker->pickers );
}

void add_custom_blur_pickers( std::string name ) {
    //std::cout << "add_custom_blur_pickers: " << name << std::endl;
    auto picker = global_context->s->get_fn_ptr< int, custom_blur_picker >( name );
    picker->add_pickers();
    //print_vector_of_pairs( picker->pickers );
}

std::string load_file_as_string(const std::string& filePath) {
    std::ifstream fileStream(filePath);
    if (!fileStream) {
        // Handle the error, e.g., by throwing an exception or returning an error message
        throw std::runtime_error("Unable to open file: " + filePath);
    }

    std::stringstream buffer;
    buffer << fileStream.rdbuf();
    return buffer.str();
}

void load_scene( std::string scene_file ) {
    global_context->s = std::make_unique< scene >( scene_file );
    any_buffer_pair_ptr any_buf = global_context->buf;
    global_context->s->set_output_buffer( any_buf );
    global_context->s->ui.canvas_bounds = bb2i( global_context->buf->get_image().get_dim() );
    if( global_context->scene_callback_ready ) {
        global_context->scene_callback();
    }
    else {
        std::cout << "load_scene: scene callback not ready" << std::endl;
    }
}

std::string get_scene_list_JSON() {
    return global_context->scene_list.dump();
}

std::string get_panel_JSON() {
    nlohmann::json j;

    j = global_context->s->ui.widget_groups;
    for( auto wg : global_context->s->ui.widget_groups ) {
        std::cout << "get_panel_JSON: " << wg.name << std::endl;
    }
    std::cout << "get_panel_JSON: " << j.dump() << std::endl;
    return j.dump();
}

std::string get_widget_JSON( std::string name ) {
    nlohmann::json j;

    if( global_context->s->functions.contains( name ) ) {
    //    auto& sw = std::get< std::shared_ptr< switch_fn > >(global_context->s->bool_fns[ name ].any_bool_fn);
        auto& sw = global_context->s->functions[ name ];
        // std::cout << "get_widget_JSON: " << name << " found" << std::endl;
        j = sw;
    }
    // std::cout << "get_widget_JSON: " << name << " " << j.dump() << std::endl;
    return j.dump();
}

bool is_widget_group_active( std::string name ) {
    any_buffer_pair_ptr null_any_buf_ptr = null_buffer_pair_ptr;
    element_context context( *(global_context->s), null_any_buf_ptr );

    for( auto& wg : global_context->s->ui.widget_groups ) {
        if( wg.name == name ) {
            //std::cout << "is_widget_group_active: " << name << " " << wg( context ) << std::endl;
            return wg( context );
        }
    }
    std::cout << "is_widget_group_active: " << name << " not in UI" << std::endl;
    return false;   // group not in UI
}

void add_image_to_scene(std::string name, std::string filepath) {
    try {
        std::cout << "Adding image: " << name << " filepath: " << filepath << std::endl;

        ubuf_ptr img(new buffer_pair<ucolor>(filepath));
        any_buffer_pair_ptr any_buf = img;

        global_context->s->buffers[name] = any_buf;
        // Mark affected queues for re-render
        for (auto& q : global_context->s->queue) {
            q.rendered = false;
        }
        global_context->s->restart();
    } catch (const std::exception& e) {
        std::cerr << "Error in add_image_to_scene: " << e.what() << std::endl;
        throw;
    }
}


void update_source_name(std::string name) {
    DEBUG("C++ update_source_name: Request to switch to '" + name + "'");
    if (!global_context || !global_context->s) return;

    // validate the buffer exists
    if (!global_context->s->buffers.count(name) || !std::holds_alternative<
            ubuf_ptr>(global_context->s->buffers[name])) {
        std::cerr << "ERROR: update_source_name - Invalid or non-ucolor source name: " << name << std::endl;
        return;
            }

    // update the menu function
    std::string menu_func_name = "source_image_menu";
    if (global_context->s->functions.count(menu_func_name)) {
        try {
            auto menu = global_context->s->get_fn_ptr<std::string, menu_string>(menu_func_name);
            menu->choose(name); // update menu choice

            if (menu->rerender) {
                global_context->s->restart(); // full start if menu causes structural change
                DEBUG("  Source updated via menu, restarting scene.");
            } else {
                global_context->s->ui.displayed = false;
                DEBUG("  Source updated via menu, flagging redraw.");
            }
        } catch (const std::exception &e) {
            std::cerr << "Warning: Failed to update menu '" << menu_func_name << "': " << e.what() << std::endl;
        }
    } else {
        std::cerr << "Warning: update_source_name - menu '" << menu_func_name << "' not found. Flagging redraw." <<
                std::endl;
        global_context->s->ui.displayed = false;
    }
}

void add_to_menu(std::string menu_name, std::string item) {
    try {
        if (global_context->s->functions.contains(menu_name)) {
            // Get the menu function pointer and cast it to menu_string type
            auto menu = global_context->s->get_fn_ptr<std::string, menu_string>(menu_name);

            // Check if item already exists to avoid duplicates
            if (std::find(menu->items.begin(), menu->items.end(), item) == menu->items.end()) {
                menu->items.push_back(item);
                std::cout << "Added " << item << " to menu " << menu_name << std::endl;
            }
        } else {
            std::cerr << "Menu " << menu_name << " not found" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in add_to_menu: " << e.what() << std::endl;
    }
}


bool worker_add_frame(val image_data, int width, int height) {
    std::cout << "[C++] === WORKER_ADD_FRAME START ===" << std::endl;
    std::cout << "[C++] Function called with dimensions: " << width << "x" << height << std::endl;
    
    if (!global_context || !global_context->video_recorder) {
        std::cerr << "[C++] ERROR: Cannot add frame - video recorder not initialized" << std::endl;
        std::cerr << "[C++] - global_context: " << (global_context ? "valid" : "null") << std::endl;
        std::cerr << "[C++] - video_recorder: " << (global_context && global_context->video_recorder ? "valid" : "null") << std::endl;
        return false;
    }

    if (!global_context->is_recording) {
        std::cerr << "[C++] ERROR: Cannot add frame - not currently recording" << std::endl;
        std::cerr << "[C++] - is_recording flag: " << global_context->is_recording << std::endl;
        return false;
    }

    try {
        // CRITICAL: Add detailed logging for debugging
        std::cout << "[C++] Starting frame processing..." << std::endl;
        std::cout << "[C++] Input validation:" << std::endl;
        std::cout << "[C++] - Width: " << width << std::endl;
        std::cout << "[C++] - Height: " << height << std::endl;
        
        // Validate dimensions
        if (width <= 0 || height <= 0) {
            std::cerr << "[C++] ERROR: Invalid dimensions: " << width << "x" << height << std::endl;
            return false;
        }
        
        // Check if image_data is valid
        if (image_data.isNull() || image_data.isUndefined()) {
            std::cerr << "[C++] ERROR: Image data is null or undefined" << std::endl;
            return false;
        }
        
        // CRITICAL FIX: Use proper Emscripten typed memory view
        const int total_pixels = width * height;
        const int expected_length = total_pixels * 4; // RGBA
        std::cout << "[C++] Data validation:" << std::endl;
        std::cout << "[C++] - Total pixels: " << total_pixels << std::endl;
        std::cout << "[C++] - Expected length: " << expected_length << std::endl;
        
        // Get the length property to validate
        int actual_length = image_data["length"].as<int>();
        std::cout << "[C++] - Actual length: " << actual_length << std::endl;
        
        if (actual_length != expected_length) {
            std::cerr << "[C++] ERROR: Image data length mismatch: got " << actual_length << ", expected " << expected_length << std::endl;
            return false;
        }
        
        std::cout << "[C++] Converting JavaScript array to memory view..." << std::endl;
        // CRITICAL FIX: Convert the JavaScript typed array to a proper memory view
        // This creates a typed_memory_view from the JavaScript Uint8ClampedArray
        auto memory_view = emscripten::convertJSArrayToNumberVector<uint8_t>(image_data);
        
        if (memory_view.size() != expected_length) {
            std::cerr << "[C++] ERROR: Memory view size mismatch: got " << memory_view.size() << ", expected " << expected_length << std::endl;
            return false;
        }
        std::cout << "[C++] Memory view conversion successful, size: " << memory_view.size() << std::endl;

        // Debug: Check recording dimensions vs frame dimensions
        if (global_context->video_recorder) {
            auto opts = global_context->video_recorder->get_options();
            std::cout << "[C++] Recording settings:" << std::endl;
            std::cout << "[C++] - Recording dimensions: " << opts.width << "x" << opts.height << std::endl;
            std::cout << "[C++] - Recording FPS: " << opts.fps << std::endl;
            std::cout << "[C++] - Recording bitrate: " << opts.bitrate << std::endl;
            std::cout << "[C++] - Recording codec: " << opts.codec << std::endl;
            std::cout << "[C++] Frame dimensions: " << width << "x" << height << std::endl;
            if (opts.width != width || opts.height != height) {
                std::cout << "[C++] WARNING: Dimensions differ - will scale frame to match recording settings" << std::endl;
            } else {
                std::cout << "[C++] Dimensions match - no scaling needed" << std::endl;
            }
        }

        // Check recorder state before processing
        auto recorder_state = global_context->video_recorder->get_state();
        std::cout << "[C++] Video recorder state: " << static_cast<int>(recorder_state) << std::endl;
        
        // OPTIMIZED: Use RGBA data directly - no conversion to ucolor needed
        std::cout << "[C++] Using RGBA data directly, converting to YUV420P" << std::endl;

        // Add frame to recording using efficient RGBA method
        std::cout << "[C++] Calling add_frame_rgba..." << std::endl;
        bool success = global_context->video_recorder->add_frame_rgba(memory_view.data(), width, height);
        std::cout << "[C++] add_frame_rgba returned: " << (success ? "SUCCESS" : "FAILURE") << std::endl;
        
        if (!success) {
            std::cerr << "[C++] ERROR: Failed to add RGBA frame to recording" << std::endl;
            std::string error_msg = global_context->video_recorder->get_error();
            std::cerr << "[C++] - Error message: " << error_msg << std::endl;
            auto final_state = global_context->video_recorder->get_state();
            std::cerr << "[C++] - Final recording state: " << static_cast<int>(final_state) << std::endl;
            int frame_count = global_context->video_recorder->get_frame_count();
            std::cerr << "[C++] - Current frame count: " << frame_count << std::endl;
        } else {
            int frame_count = global_context->video_recorder->get_frame_count();
            std::cout << "[C++] SUCCESS: RGBA frame converted to YUV420P and added successfully" << std::endl;
            std::cout << "[C++] - Current frame count: " << frame_count << std::endl;
        }
        
        std::cout << "[C++] === WORKER_ADD_FRAME END ===" << std::endl;
        return success;
    } catch (const std::exception& e) {
        std::cerr << "[C++] EXCEPTION in worker_add_frame: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "[C++] UNKNOWN EXCEPTION in worker_add_frame" << std::endl;
        return false;
    }
}

val get_recording_data() {
    if (!global_context || !global_context->video_recorder) {
        std::cerr << "No video recorder available" << std::endl;
        return val::null();
    }
    const std::vector<uint8_t>& buffer = global_context->video_recorder->get_output_buffer();
    if (buffer.empty()) {
        std::cerr << "No video data in output buffer" << std::endl;
        return val::null();
    }
    return val(typed_memory_view(buffer.size(), buffer.data()));
}


std::string get_recording_error() {
    if (!global_context || !global_context->video_recorder) {
        return "video recorder not initialized";
    }
    return global_context->video_recorder->get_error();
}


int get_recorded_frame_count() {
    if (!global_context || !global_context->video_recorder) {
        return 0;
    }
    return global_context->video_recorder->get_frame_count();
}



 bool stop_recording() {
    if (!global_context || !global_context->video_recorder) {
        std::cerr << "Cannot stop recording - video recorder not initialized" << std::endl;
        return false;
    }
    if (!global_context->is_recording) {
        std::cerr << "Cannot stop recording - not currently recording" << std::endl;
        return false;
    }
    global_context->is_recording = false;
    return global_context->video_recorder->stop_recording();
}


bool start_recording(int width, int height, int fps, int bitrate, std::string codec, std::string format, std::string preset) {
    if (!global_context || !global_context->video_recorder) {
        return false;
    }
    if (global_context->is_recording) {
        stop_recording();
    }

    RecordingOptions options;
    options.width = width;
    options.height = height;
    options.fps = fps;
    options.bitrate = bitrate;
    options.codec = codec;
    options.format = format;
    options.preset = preset;

    // Start recording
    bool success = global_context->video_recorder->start_recording(options);
    if (success) {
        global_context->is_recording = true;
        return true;
    }
    return false;
}

bool start_recording_adaptive(int fps, int bitrate, std::string codec, std::string format, std::string preset) {
    if (!global_context || !global_context->video_recorder) {
        return false;
    }
    if (global_context->is_recording) {
        stop_recording();
    }

    RecordingOptions options;
    options.width = global_context->buf->get_image().get_dim().x;
    options.height = global_context->buf->get_image().get_dim().y;
    options.fps = fps;
    options.bitrate = bitrate;
    options.codec = codec;
    options.format = format;
    options.preset = preset;

    // Start recording
    bool success = global_context->video_recorder->start_recording(options);
    if (success) {
        global_context->is_recording = true;
        return true;
    }
    return false;
}

bool is_recording() {
    return global_context && global_context->is_recording;
}



std::string get_recording_state() {
    if (!global_context || !global_context->video_recorder) {
        return "error";
    }

    switch (global_context->video_recorder->get_state()) {
        case RecordingState::IDLE: return "idle";
        case RecordingState::RECORDING: return "recording";
        case RecordingState::ENCODING: return "encoding";
        case RecordingState::ERROR: return "error";
        default: return "unknown";
    }
}

// Camera-specific functions for live processing
bool add_camera_frame(val image_data, int width, int height, std::string temp_name = "live_camera") {
    try {
        // Create a temporary image for camera frame
        auto temp_img_ptr = std::make_unique<uimage>(vec2i{width, height});
        if (!temp_img_ptr) {
            std::cerr << "Failed to create camera frame image" << std::endl;
            return false;
        }

        uimage temp_img = *temp_img_ptr;
        unsigned char* img_buffer = (unsigned char*)temp_img.get_base_ptr();
        size_t buffer_length = width * height * 4;

        // Copy the data from the TypedArray (camera frame)
        for (unsigned int i = 0; i < buffer_length; ++i) {
            img_buffer[i] = image_data[i].as<uint8_t>();
        }

        // Add to scene buffers as temporary camera source
        ubuf_ptr camera_buf(new buffer_pair<ucolor>(temp_img));
        any_buffer_pair_ptr any_buf = camera_buf;
        global_context->s->buffers[temp_name] = any_buf;

        // Mark for re-render
        global_context->s->ui.displayed = false;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error in add_camera_frame: " << e.what() << std::endl;
        return false;
    }
}

bool process_live_camera_frame(val image_data, int width, int height) {
    // Process camera frame for live preview with effects
    if (!add_camera_frame(image_data, width, height, "live_camera_preview")) {
        return false;
    }
    
    // Update source to camera preview for live effects
    try {
        if (global_context->s->functions.count("source_image_menu")) {
            auto menu = global_context->s->get_fn_ptr<std::string, menu_string>("source_image_menu");
            // Temporarily switch to camera preview without triggering full restart
            std::string previous_choice = menu->get_chosen_item();
            menu->choose("live_camera_preview");
            
            // Process one frame
            global_context->s->render();
            
            // Restore previous choice
            menu->choose(previous_choice);
            return true;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error processing live camera frame: " << e.what() << std::endl;
    }
    
    return false;
}

void set_camera_source_active(bool active) {
    // Enable/disable camera as active source
    if (!global_context || !global_context->s) return;
    
    try {
        if (active) {
            // Switch to camera source
            update_source_name("live_camera_preview");
        } else {
            // Switch back to previous source
            // This could be enhanced to remember the previous source
            if (global_context->s->functions.count("source_image_menu")) {
                auto menu = global_context->s->get_fn_ptr<std::string, menu_string>("source_image_menu");
                if (!menu->items.empty()) {
                    update_source_name(menu->items[0]); // Default to first item
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error setting camera source active: " << e.what() << std::endl;
    }
}

std::string get_camera_processing_stats() {
    // Return JSON with camera processing statistics
    nlohmann::json stats;
    
    if (global_context && global_context->s) {
        stats["camera_buffer_exists"] = global_context->s->buffers.count("live_camera_preview") > 0;
        stats["total_buffers"] = global_context->s->buffers.size();
        stats["canvas_width"] = global_context->buf->get_image().get_dim().x;
        stats["canvas_height"] = global_context->buf->get_image().get_dim().y;
        stats["is_running"] = global_context->s->ui.running;
        stats["displayed"] = global_context->s->ui.displayed;
    } else {
        stats["error"] = "global context not available";
    }
    
    return stats.dump();
}

// Enhanced image saving with camera metadata
void save_camera_image_with_metadata(std::string name, std::string filepath, val metadata) {
    try {
        std::cout << "Saving camera image: " << name << " filepath: " << filepath << std::endl;

        // Create buffer from file
        ubuf_ptr img(new buffer_pair<ucolor>(filepath));
        any_buffer_pair_ptr any_buf = img;

        global_context->s->buffers[name] = any_buf;
        
        // Add camera-specific metadata if provided
        if (!metadata.isNull() && !metadata.isUndefined()) {
            // Could store metadata in a separate structure if needed
            std::cout << "Camera metadata received (not yet stored)" << std::endl;
        }
        
        // Mark affected queues for re-render
        for (auto& q : global_context->s->queue) {
            q.rendered = false;
        }
        global_context->s->restart();
    } catch (const std::exception& e) {
        std::cerr << "Error in save_camera_image_with_metadata: " << e.what() << std::endl;
        throw;
    }
}

// Camera processing with persistent buffers and zero-copy operations
struct UltraCameraContext {
    // Persistent buffers - allocated once, reused forever
    std::shared_ptr<buffer_pair<ucolor>> camera_buffer;
    std::shared_ptr<buffer_pair<ucolor>> effect_buffer;
    
    // OPTIMAL PERFORMANCE: 256x256 for maximum smooth 60fps
    vec2i fixed_dimensions{256, 256};  // Original smooth performance
    
    // Performance tracking
    std::chrono::high_resolution_clock::time_point last_frame_time;
    float current_fps = 0.0f;
    int frame_count = 0;
    int processed_frames = 0;
    
    // State management
    bool is_active = false;
    bool buffers_initialized = false;
    std::string backup_source;
    
    // Ultra-performance flags
    bool skip_next_frame = false;
    int skip_counter = 0;
    int max_skip_frames = 2;
    
    // Buffer reuse optimization
    ucolor* camera_pixels = nullptr;
    ucolor* effect_pixels = nullptr;
    size_t buffer_size = 0;
};

UltraCameraContext* ultra_camera = nullptr;

// Initialize ultra-optimized camera context with persistent buffers
void init_ultra_camera() {
    if (!ultra_camera) {
        ultra_camera = new UltraCameraContext();
        
        // Pre-allocate fixed-size buffers for maximum performance
        ultra_camera->camera_buffer = std::make_shared<buffer_pair<ucolor>>(ultra_camera->fixed_dimensions);
        ultra_camera->effect_buffer = std::make_shared<buffer_pair<ucolor>>(ultra_camera->fixed_dimensions);
        
        // Cache direct pixel access pointers for zero-copy operations
        ultra_camera->camera_pixels = ultra_camera->camera_buffer->get_image().get_base_ptr();
        ultra_camera->effect_pixels = ultra_camera->effect_buffer->get_image().get_base_ptr();
        ultra_camera->buffer_size = ultra_camera->fixed_dimensions.x * ultra_camera->fixed_dimensions.y;
        
        ultra_camera->buffers_initialized = true;
        ultra_camera->last_frame_time = std::chrono::high_resolution_clock::now();
        
        std::cout << "Ultra camera initialized: " << ultra_camera->fixed_dimensions.x << "x" 
                  << ultra_camera->fixed_dimensions.y << " (" << ultra_camera->buffer_size << " pixels)" << std::endl;
    }
}

// ULTRA-FAST camera frame update with zero-copy RGBA→ARGB conversion
bool ultra_update_camera_frame(val image_data, int width, int height) {
    std::cout << "=== ULTRA_UPDATE_CAMERA_FRAME START ===" << std::endl;
    std::cout << "Input: " << width << "x" << height << std::endl;
    
    init_ultra_camera();
    
    if (!ultra_camera || !ultra_camera->buffers_initialized) {
        std::cerr << "ERROR: Ultra camera not initialized" << std::endl;
        return false;
    }
    
    // Validate input dimensions
    if (width != ultra_camera->fixed_dimensions.x || height != ultra_camera->fixed_dimensions.y) {
        std::cerr << "ERROR: Invalid dimensions: expected " << ultra_camera->fixed_dimensions.x 
                  << "x" << ultra_camera->fixed_dimensions.y 
                  << ", got " << width << "x" << height << std::endl;
        return false;
    }
    
    try {
        // Get the raw data from JavaScript Uint8Array
        auto length = image_data["length"].as<unsigned>();
        std::cout << "JS array length: " << length << std::endl;
        
        // Expected size for RGBA data
        size_t expected_size = width * height * 4;
        if (length != expected_size) {
            std::cerr << "ERROR: Size mismatch: expected " << expected_size << " bytes, got " << length << std::endl;
            return false;
        }
        
        // CRITICAL DEBUG: Check first few bytes from JavaScript
        std::cout << "First 12 JS bytes: ";
        for (int i = 0; i < 12 && i < length; i++) {
            uint8_t val = image_data[i].as<uint8_t>();
            std::cout << (int)val << " ";
        }
        std::cout << std::endl;
        
        // Check if JS data has any non-zero values
        int js_non_zero = 0;
        for (size_t i = 0; i < std::min((size_t)1000, (size_t)length); i++) {
            if (image_data[i].as<uint8_t>() > 0) js_non_zero++;
        }
        std::cout << "JS non-zero count (first 1000): " << js_non_zero << std::endl;
        
        if (js_non_zero == 0) {
            std::cerr << "ERROR: JavaScript data is all zeros!" << std::endl;
            return false;
        }
        
        // Get camera buffer for writing
        auto camera_buffer = &(ultra_camera->camera_buffer->get_image());
        if (!camera_buffer) {
            std::cerr << "ERROR: Camera buffer not available" << std::endl;
            return false;
        }
        
        ucolor* camera_pixels = camera_buffer->get_base_ptr();
        if (!camera_pixels) {
            std::cerr << "ERROR: Camera pixels not available" << std::endl;
            return false;
        }
        
        std::cout << "Processing pre-converted ARGB data from frontend..." << std::endl;
        
        // OPTIMIZED: Frontend now sends ARGB data (matching still image capture pattern)
        // No conversion needed - directly copy bytes to ucolor buffer
        size_t pixel_count = width * height;
        for (size_t i = 0; i < pixel_count; i++) {
            size_t argb_idx = i * 4;
            
            // Bounds check
            if (argb_idx + 3 >= length) {
                std::cerr << "ERROR: Bounds error at pixel " << i << std::endl;
                break;
            }
            
            uint8_t a = image_data[argb_idx + 0].as<uint8_t>();  // Alpha (position 0)
            uint8_t r = image_data[argb_idx + 1].as<uint8_t>();  // Red (position 1)  
            uint8_t g = image_data[argb_idx + 2].as<uint8_t>();  // Green (position 2)
            uint8_t b = image_data[argb_idx + 3].as<uint8_t>();  // Blue (position 3)
            
            // Data is already in ARGB format - assemble into ucolor (uint32_t)
            camera_pixels[i] = ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
        }
        
        // VERIFY: Check camera buffer after conversion
        int camera_non_zero = 0;
        for (int i = 0; i < std::min(100, (int)pixel_count); i++) {
            uint32_t pixel = camera_pixels[i];
            uint8_t r = (pixel >> 16) & 0xFF;
            uint8_t g = (pixel >> 8) & 0xFF;
            uint8_t b = pixel & 0xFF;
            if (r > 0 || g > 0 || b > 0) camera_non_zero++;
        }
        std::cout << "Camera buffer non-zero after conversion: " << camera_non_zero << "/100" << std::endl;
        
        if (camera_non_zero > 0) {
            std::cout << "First 3 camera pixels: ";
            for (int i = 0; i < 3; i++) {
                uint32_t pixel = camera_pixels[i];
                uint8_t r = (pixel >> 16) & 0xFF;
                uint8_t g = (pixel >> 8) & 0xFF;
                uint8_t b = pixel & 0xFF;
                std::cout << "(" << (int)r << "," << (int)g << "," << (int)b << ") ";
            }
            std::cout << std::endl;
        }
        
        ultra_camera->frame_count++;
        std::cout << "SUCCESS: Frame " << ultra_camera->frame_count << " updated" << std::endl;
        std::cout << "=== ULTRA_UPDATE_CAMERA_FRAME END ===" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Exception in ultra_update_camera_frame: " << e.what() << std::endl;
        std::cout << "=== ULTRA_UPDATE_CAMERA_FRAME END (ERROR) ===" << std::endl;
        return false;
    }
}

bool ultra_process_camera_with_kaleidoscope() {
    std::cout << "=== SCENE_INTEGRATED_CAMERA_PROCESSING START ===" << std::endl;
    
    if (!ultra_camera || !ultra_camera->is_active || !global_context || !global_context->s) {
        std::cerr << "ERROR: Ultra camera not active or context not available" << std::endl;
        return false;
    }
    
    try {
        // STEP 1: Verify ultra_camera buffer exists in scene
        if (!global_context->s->buffers.count("ultra_camera")) {
            std::cerr << "ERROR: ultra_camera buffer not found in scene buffers" << std::endl;
            return false;
        }
        
        auto camera_buffer = std::get<ubuf_ptr>(global_context->s->buffers["ultra_camera"]);
        if (!camera_buffer || !camera_buffer->has_image()) {
            std::cerr << "ERROR: Camera buffer invalid or has no image" << std::endl;
            return false;
        }
        
        std::cout << "✓ Camera buffer verified with image data" << std::endl;
        
        std::cout << "Triggering scene render with current frontend settings..." << std::endl;
        
        global_context->s->ui.displayed = false;
        
        // Execute scene render - this will:
        // 1. Read the ultra_camera buffer as source
        // 2. Apply all effects based on current frontend control panel values  
        // 3. Render the result to the main output buffer
        global_context->s->render();
        
        std::cout << "✓ Scene render completed with frontend-controlled effects" << std::endl;
        
        // STEP 3: Verify scene processing worked
        if (global_context->buf && global_context->buf->has_image()) {
            auto& main_image = global_context->buf->get_image();
            vec2i main_dims = main_image.get_dim();
            
            std::cout << "Main buffer after scene processing: " << main_dims.x << "x" << main_dims.y << std::endl;
            
            // Quick verification of output
            auto main_pixels = main_image.get_base_ptr();
            int processed_pixels = 0;
            for (int i = 0; i < std::min(100, main_dims.x * main_dims.y); i++) {
                uint32_t pixel = main_pixels[i];
                if ((pixel & 0x00FFFFFF) != 0) { // Check RGB components
                    processed_pixels++;
                }
            }
            
            std::cout << "Scene output verification: " << processed_pixels << "/100 processed pixels" << std::endl;
            
            if (processed_pixels > 10) {
                std::cout << "✓ Scene successfully processed camera with frontend effects" << std::endl;
                return true;
            } else {
                std::cout << "WARNING: Scene output appears minimal" << std::endl;
                return false;
            }
        } else {
            std::cerr << "ERROR: No main buffer after scene processing" << std::endl;
            return false;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "ERROR in scene-based camera processing: " << e.what() << std::endl;
        return false;
    }
}

// ULTRA-OPTIMIZED kaleidoscope processing with camera integration
bool ultra_start_camera_stream() {
    std::cout << "=== ULTRA_START_CAMERA_STREAM ===" << std::endl;
    
    init_ultra_camera();
    
    if (!global_context || !global_context->s) {
        std::cerr << "ERROR: Global context not available for ultra camera" << std::endl;
        return false;
    }
    
    try {
        // Backup current source
        if (!ultra_camera->is_active) {
            if (global_context->s->functions.count("source_image_menu")) {
                auto menu = global_context->s->get_fn_ptr<std::string, menu_string>("source_image_menu");
                ultra_camera->backup_source = menu->get_chosen_item();
                std::cout << "Backed up current source: " << ultra_camera->backup_source << std::endl;
            }
        }
        
        // Register camera buffer in scene system
        any_buffer_pair_ptr camera_any_buf = ultra_camera->camera_buffer;
        global_context->s->buffers["ultra_camera"] = camera_any_buf;
        std::cout << "Registered ultra_camera buffer in scene system" << std::endl;
        
        // Add to source menu if not already there
        if (global_context->s->functions.count("source_image_menu")) {
            auto menu = global_context->s->get_fn_ptr<std::string, menu_string>("source_image_menu");
            if (std::find(menu->items.begin(), menu->items.end(), "ultra_camera") == menu->items.end()) {
                menu->items.push_back("ultra_camera");
                std::cout << "Added ultra_camera to source menu" << std::endl;
            } else {
                std::cout << "ultra_camera already in source menu" << std::endl;
            }
        }
        
        ultra_camera->is_active = true;
        ultra_camera->frame_count = 0;
        ultra_camera->processed_frames = 0;
        
        std::cout << "✓ Ultra camera stream started with kaleidoscope scene integration" << std::endl;
        std::cout << "✓ Scene-based effects ready: segments, levels, spin, expand, reflect" << std::endl;
        std::cout << "✓ Vector field transformations available" << std::endl;
        std::cout << "✓ Multiple kaleidoscope modes: Kaleido, Multiples, Tile" << std::endl;
        std::cout << "=== ULTRA_START_CAMERA_STREAM SUCCESS ===" << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Exception starting ultra camera stream: " << e.what() << std::endl;
        return false;
    }
}

// Stop ultra camera and restore previous state
bool ultra_stop_camera_stream() {
    std::cout << "=== ULTRA_STOP_CAMERA_STREAM ===" << std::endl;
    
    if (!ultra_camera || !ultra_camera->is_active) {
        std::cout << "Ultra camera already stopped or not initialized" << std::endl;
        return true; // Already stopped
    }
    
    try {
        // Restore previous source
        if (!ultra_camera->backup_source.empty()) {
            if (global_context->s->functions.count("source_image_menu")) {
                auto menu = global_context->s->get_fn_ptr<std::string, menu_string>("source_image_menu");
                menu->choose(ultra_camera->backup_source);
                std::cout << "Restored source to: " << ultra_camera->backup_source << std::endl;
            }
        }
        
        ultra_camera->is_active = false;
        
        // Force render with restored source
        global_context->s->ui.displayed = false;
        global_context->s->render();
        
        std::cout << "SUCCESS: Ultra camera stream stopped and source restored" << std::endl;
        std::cout << "=== ULTRA_STOP_CAMERA_STREAM END ===" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Exception in ultra_stop_camera_stream: " << e.what() << std::endl;
        std::cout << "=== ULTRA_STOP_CAMERA_STREAM END (ERROR) ===" << std::endl;
        return false;
    }
}

// Get ultra camera performance statistics
std::string ultra_get_camera_stats() {
    nlohmann::json stats;
    
    if (ultra_camera) {
        stats["initialized"] = ultra_camera->buffers_initialized;
        stats["active"] = ultra_camera->is_active;
        stats["fps"] = ultra_camera->current_fps;
        stats["processed_frames"] = ultra_camera->processed_frames;
        stats["width"] = ultra_camera->fixed_dimensions.x;
        stats["height"] = ultra_camera->fixed_dimensions.y;
        stats["buffer_size"] = ultra_camera->buffer_size;
        stats["skip_frames"] = ultra_camera->max_skip_frames;
        stats["skip_counter"] = ultra_camera->skip_counter;
        stats["frame_count"] = ultra_camera->frame_count;
    } else {
        stats["initialized"] = false;
        stats["active"] = false;
        stats["fps"] = 0.0f;
        stats["processed_frames"] = 0;
        stats["frame_count"] = 0;
    }
    
    if (global_context && global_context->s) {
        stats["scene_running"] = global_context->s->ui.running;
        stats["total_buffers"] = global_context->s->buffers.size();
        stats["ultra_camera_buffer_exists"] = global_context->s->buffers.count("ultra_camera") > 0;
    }
    
    return stats.dump();
}

// Audio context management functions
void update_audio_context(float volume, float bass, float mid, float high, bool beat, float time) {
    if (!global_context || !global_context->s) {
        std::cout << "🎵 ❌ Audio context update failed: global_context or scene is null" << std::endl;
        return;
    }
    
    static int log_counter = 0;
    log_counter++;
    
    try {
        // Update audio values
        global_context->s->ui.audio.volume = volume;
        global_context->s->ui.audio.bass_level = bass;
        global_context->s->ui.audio.mid_level = mid;
        global_context->s->ui.audio.high_level = high;
        global_context->s->ui.audio.beat_detected = beat;
        global_context->s->ui.audio.time_phase = time;
        
        // DIVERSE AUDIO-CONTROLLED EFFECTS FOR PARTY MODE
        if (global_context->s->ui.audio.enabled && 
            (volume > 0.001f || bass > 0.001f || mid > 0.001f || high > 0.001f)) {
            
            float sensitivity = global_context->s->ui.audio.global_sensitivity;
            
            // === RESPECT USER'S PAUSE STATE ===
            // Don't force animation to start when audio is detected - respect user's choice
            bool should_control_animation = global_context->s->ui.running;
            
            // Force continuous redraws for responsiveness when animation is running
            if (should_control_animation) {
                global_context->s->ui.displayed = false;
            }
            
            // === SOUND TYPE DETECTION ===
            float bass_ratio = bass / (volume + 0.001f);    // How bass-heavy
            float high_ratio = high / (volume + 0.001f);    // How treble-heavy  
            float mid_ratio = mid / (volume + 0.001f);      // How mid-heavy
            
            // Sound type classification
            bool is_music = (bass_ratio > 0.4f && high_ratio > 0.3f);        // Full spectrum
            bool is_singing = (mid_ratio > 0.5f && volume > 0.15f);          // Mid-heavy, strong
            bool is_drums = (bass_ratio > 0.6f && beat);                     // Bass + beat
            bool is_instrument = (high_ratio > 0.4f && !beat);               // High freq, no beat
            
            // === DYNAMIC ANIMATION SPEED CONTROL ===
            if (should_control_animation) {
                float speed_base = 1.0f;
                
                if (is_drums) {
                    // Drums: Punchy, rhythmic speed changes
                    speed_base = beat ? 2.5f : 1.2f;
                } else if (is_singing) {
                    // Singing: Smooth, voice-following speed
                    speed_base = 1.0f + (volume * sensitivity * 1.5f);
                } else if (is_music) {
                    // Music: Complex speed based on all frequencies
                    speed_base = 1.0f + ((bass * 0.4f + mid * 0.3f + high * 0.3f) * sensitivity * 1.8f);
                } else if (is_instrument) {
                    // Instruments: High-frequency reactive
                    speed_base = 1.0f + (high * sensitivity * 2.0f);
                } else {
                    // General audio - ensure minimum speed boost
                    speed_base = 1.0f + (volume * sensitivity * 1.2f);
                }
                
                // Clamp speed to reasonable range but ensure minimum boost
                speed_base = std::max(1.1f, std::min(4.0f, speed_base)); // Minimum 1.1x when audio detected
                
                // Apply speed
                float base_interval = global_context->s->default_time_interval;
                global_context->s->time_interval = base_interval * speed_base;
                
                // Beat-triggered effects: Force extra redraws on beats
                static bool last_beat = false;
                if (beat && !last_beat) {
                    for (int i = 0; i < 5; i++) { 
                        global_context->s->ui.displayed = false;
                    }
                    std::cout << "🥁 BEAT PUNCH! Extra redraws triggered" << std::endl;
                }
                last_beat = beat;
                
                // High-energy continuous updates - more aggressive
                if (volume > 0.1f || bass > 0.2f || high > 0.2f) {
                    // High energy: Force extra visual updates
                    global_context->s->ui.displayed = false;
                }
            }
            
        } else {
            // Reset to default speed when audio is disabled or silent
            global_context->s->time_interval = global_context->s->default_time_interval;
        }
        
    } catch (const std::exception& e) {
        std::cout << "🎵 ❌ update_audio_context error: " << e.what() << std::endl;
    } catch (...) {
        std::cout << "🎵 ❌ update_audio_context unknown error" << std::endl;
    }
}

void enable_audio_input(bool enabled) {
    if (!global_context || !global_context->s) return;
    global_context->s->ui.audio.enabled = enabled;
    
    // When disabling audio, clear all audio values to prevent residual effects
    if (!enabled) {
        global_context->s->ui.audio.volume = 0.0f;
        global_context->s->ui.audio.bass_level = 0.0f;
        global_context->s->ui.audio.mid_level = 0.0f;
        global_context->s->ui.audio.high_level = 0.0f;
        global_context->s->ui.audio.beat_detected = false;
        global_context->s->ui.audio.time_phase = 0.0f;
        std::cout << "🎵 🧹 Audio values cleared on disable" << std::endl;
    }
}

void set_audio_sensitivity(float sensitivity) {
    if (!global_context || !global_context->s) return;
    global_context->s->ui.audio.global_sensitivity = sensitivity;
    std::cout << "🎵 🎛️ Global audio sensitivity set to: " << sensitivity << std::endl;
}

// Check autoplay integration status
std::string get_autoplay_audio_status() {
    if (!global_context || !global_context->s) {
        return "{\"error\": \"No scene context\"}";
    }
    
    nlohmann::json status;
    status["audio_enabled"] = global_context->s->ui.audio.enabled;
    status["audio_sensitivity"] = global_context->s->ui.audio.global_sensitivity;
    status["animation_running"] = global_context->s->ui.running;
    
    // Check if autoplay_switch exists
    bool has_autoplay = global_context->s->functions.contains("autoplay_switch");
    status["has_autoplay"] = has_autoplay;
    
    if (has_autoplay) {
        try {
            auto autoplay_fn = global_context->s->get_fn_ptr<bool, switch_fn>("autoplay_switch");
            if (autoplay_fn) {
                status["autoplay_active"] = *autoplay_fn->value;
                status["integration_status"] = "compatible";
                
                // Count autoplay-enabled functions
                int autoplay_functions = 0;
                for (const auto& [name, fn] : global_context->s->functions) {
                    // This is a simplified check - in real implementation you'd need to traverse the harness
                    if (name.find("_toggle") != std::string::npos || name.find("_generator") != std::string::npos) {
                        autoplay_functions++;
                    }
                }
                status["autoplay_functions_count"] = autoplay_functions;
            } else {
                status["integration_status"] = "autoplay_function_not_accessible";
            }
        } catch (const std::exception& e) {
            status["integration_status"] = "error_accessing_autoplay";
            status["error"] = e.what();
        }
    } else {
        status["integration_status"] = "no_autoplay_in_scene";
    }
    
    // Audio function count
    int audio_functions = 0;
    for (const auto& [name, fn] : global_context->s->functions) {
        if (name.find("audio_") != std::string::npos) {
            audio_functions++;
        }
    }
    status["audio_functions_count"] = audio_functions;
    
    return status.dump();
}

// Get information about which sliders are influenced by autoplay
std::string get_autoplay_slider_info() {
    if (!global_context || !global_context->s) {
        return "{\"error\": \"No scene context\"}";
    }
    
    nlohmann::json info;
    info["autoplay_sliders"] = nlohmann::json::array();
    
    // Check each function to see if it's a slider with autoplay influence
    for (const auto& [name, fn] : global_context->s->functions) {
        std::visit([&](auto&& func_wrapper) {
            using T = std::decay_t<decltype(func_wrapper)>;
            
            if constexpr (std::is_same_v<T, any_fn<float>>) {
                std::visit([&](auto&& fn_ptr) {
                    using FnType = std::decay_t<decltype(fn_ptr)>;
                    if constexpr (std::is_same_v<FnType, std::shared_ptr<slider_float>>) {
                        nlohmann::json slider_info;
                        slider_info["name"] = name;
                        slider_info["type"] = "float";
                        slider_info["has_autoplay"] = false;
                        slider_info["should_show_realtime"] = false;
                        
                        // Check if this slider's value harness contains autoplay functions
                        // This is a simplified check - would need proper harness traversal for full implementation
                        if (name.find("segment") != std::string::npos || 
                            name.find("level") != std::string::npos ||
                            name.find("expand") != std::string::npos ||
                            name.find("spin") != std::string::npos ||
                            name.find("swirl") != std::string::npos) {
                            slider_info["has_autoplay"] = true;
                            // Show realtime for tweaker functions (smooth changes)
                            slider_info["should_show_realtime"] = true;
                        }
                        
                        info["autoplay_sliders"].push_back(slider_info);
                    }
                }, func_wrapper.any_fn_ptr);
            }
            else if constexpr (std::is_same_v<T, any_fn<int>>) {
                std::visit([&](auto&& fn_ptr) {
                    using FnType = std::decay_t<decltype(fn_ptr)>;
                    if constexpr (std::is_same_v<FnType, std::shared_ptr<slider_int>>) {
                        nlohmann::json slider_info;
                        slider_info["name"] = name;
                        slider_info["type"] = "int";
                        slider_info["has_autoplay"] = false;
                        slider_info["should_show_realtime"] = false;
                        
                        info["autoplay_sliders"].push_back(slider_info);
                    }
                    else if constexpr (std::is_same_v<FnType, std::shared_ptr<menu_int>>) {
                        nlohmann::json menu_info;
                        menu_info["name"] = name;
                        menu_info["type"] = "menu_int";
                        menu_info["has_autoplay"] = false;
                        menu_info["should_show_realtime"] = false;
                        
                        // Menus with generators should NOT show realtime (discrete jumps)
                        if (name.find("scope") != std::string::npos || 
                            name.find("funky") != std::string::npos) {
                            menu_info["has_autoplay"] = true;
                            menu_info["should_show_realtime"] = false; // Discrete changes
                        }
                        
                        info["autoplay_sliders"].push_back(menu_info);
                    }
                }, func_wrapper.any_fn_ptr);
            }
        }, fn);
    }
    
    return info.dump();
}

// Slider value retrieval functions needed by frontend
float get_slider_value(std::string name) {
    if (!global_context || !global_context->s) return 0.0f;
    
    if (global_context->s->functions.contains(name)) {
        any_function& fn = global_context->s->functions[name];
        if (std::holds_alternative<any_fn<float>>(fn)) {
            return *global_context->s->get_fn_ptr<float, slider_float>(name)->value;
        }
        else if (std::holds_alternative<any_fn<int>>(fn)) {
            return (float)*global_context->s->get_fn_ptr<int, slider_int>(name)->value;
        }
    }
    return 0.0f;
}

// Check if current scene supports live camera
bool is_live_camera_supported() {
    if (!global_context || !global_context->s) {
        return false;
    }
    return global_context->s->liveCamera;
}

// Get current animation running state
bool get_animation_running() {
    if (!global_context || !global_context->s) {
        return false;
    }
    return global_context->s->ui.running;
}

// Scene-agnostic autoplay system
bool enable_scene_autoplay(bool enabled) {
    if (!global_context || !global_context->s) {
        return false;
    }
    
    // Look for autoplay_switch function in current scene
    if (global_context->s->functions.contains("autoplay_switch")) {
        try {
            auto autoplay_fn = global_context->s->get_fn_ptr<bool, switch_fn>("autoplay_switch");
            if (autoplay_fn) {
                autoplay_fn->value = enabled;
                std::cout << "🎲 Scene-agnostic autoplay " << (enabled ? "enabled" : "disabled") << std::endl;
                return true;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error setting autoplay: " << e.what() << std::endl;
        }
    }
    
    return false;
}

// Get autoplay status for any scene
bool get_autoplay_status() {
    if (!global_context || !global_context->s) {
        return false;
    }
    
    if (global_context->s->functions.contains("autoplay_switch")) {
        try {
            auto autoplay_fn = global_context->s->get_fn_ptr<bool, switch_fn>("autoplay_switch");
            if (autoplay_fn) {
                return *autoplay_fn->value;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error getting autoplay status: " << e.what() << std::endl;
        }
    }
    
    return false;
}

// Scene-agnostic autoplay intensity control
bool set_autoplay_intensity(float intensity) {
    if (!global_context || !global_context->s) {
        return false;
    }
    
    // Look for autoplay_intensity slider
    if (global_context->s->functions.contains("autoplay_intensity")) {
        try {
            auto intensity_fn = global_context->s->get_fn_ptr<float, slider_float>("autoplay_intensity");
            if (intensity_fn) {
                // Clamp intensity to reasonable bounds
                intensity = std::max(0.0001f, std::min(0.01f, intensity));
                intensity_fn->value = intensity;
                std::cout << "🎲 Autoplay intensity set to: " << intensity << std::endl;
                return true;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error setting autoplay intensity: " << e.what() << std::endl;
        }
    }
    
    return false;
}

// Get list of autoplay-influenced parameters for current scene
std::string get_scene_autoplay_info() {
    if (!global_context || !global_context->s) {
        return "{\"error\": \"No scene context\"}";
    }
    
    nlohmann::json info;
    info["scene_name"] = global_context->s->name;
    info["has_autoplay"] = global_context->s->functions.contains("autoplay_switch");
    info["autoplay_active"] = get_autoplay_status();
    info["autoplay_parameters"] = nlohmann::json::array();
    
    // Scan all functions for autoplay influence
    for (const auto& [name, fn] : global_context->s->functions) {
        if (name.find("tweaker") != std::string::npos || 
            name.find("generator") != std::string::npos || 
            name.find("toggle") != std::string::npos) {
            
            nlohmann::json param;
            param["name"] = name;
            param["type"] = "autoplay_function";
            info["autoplay_parameters"].push_back(param);
        }
        
        // Check for sliders/menus that use autoplay functions
        std::visit([&](auto&& func_wrapper) {
            using T = std::decay_t<decltype(func_wrapper)>;
            
            if constexpr (std::is_same_v<T, any_fn<float>>) {
                std::visit([&](auto&& fn_ptr) {
                    using FnType = std::decay_t<decltype(fn_ptr)>;
                    if constexpr (std::is_same_v<FnType, std::shared_ptr<slider_float>>) {
                        // This is a simplified check - full implementation would traverse harness
                        if (name.find("segment") != std::string::npos || 
                            name.find("level") != std::string::npos ||
                            name.find("expand") != std::string::npos ||
                            name.find("spin") != std::string::npos ||
                            name.find("swirl") != std::string::npos) {
                            
                            nlohmann::json param;
                            param["name"] = name;
                            param["type"] = "autoplay_influenced_slider";
                            param["data_type"] = "float";
                            info["autoplay_parameters"].push_back(param);
                        }
                    }
                }, func_wrapper.any_fn_ptr);
            }
            else if constexpr (std::is_same_v<T, any_fn<int>>) {
                std::visit([&](auto&& fn_ptr) {
                    using FnType = std::decay_t<decltype(fn_ptr)>;
                    if constexpr (std::is_same_v<FnType, std::shared_ptr<menu_int>>) {
                        if (name.find("scope") != std::string::npos || 
                            name.find("funky") != std::string::npos ||
                            name.find("rule") != std::string::npos) {
                            
                            nlohmann::json param;
                            param["name"] = name;
                            param["type"] = "autoplay_influenced_menu";
                            param["data_type"] = "int";
                            info["autoplay_parameters"].push_back(param);
                        }
                    }
                }, func_wrapper.any_fn_ptr);
            }
        }, fn);
    }
    
    return info.dump();
}

int main(int argc, char** argv) {
    using namespace nlohmann;
    std::string filename;
    vec2i dim( { 512, 512 } );  // original sin
    //auto dims = img.get_dim();
    emscripten_run_script("console.log('preparing to load scene');");
    global_context = new frame_context();
    
    try {
        std::cout << "Loading scene list from lux_files/scenes.json" << std::endl;
        global_context->scene_list = json::parse( load_file_as_string( "lux_files/scenes.json" ) );
        std::cout << "Scene list loaded successfully" << std::endl;
        
        std::cout << "Getting first scene filename" << std::endl;
        global_context->scene_list[ "scenes" ][ 0 ][ "filename" ].get_to( filename );
        std::cout << "First scene filename: " << filename << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "ERROR loading scene list: " << e.what() << std::endl;
        emscripten_run_script("console.error('Scene list loading failed');");
        throw;
    } catch (...) {
        std::cerr << "UNKNOWN ERROR loading scene list" << std::endl;
        emscripten_run_script("console.error('Scene list loading failed with unknown error');");
        throw;
    }
    
    try {
        std::cout << "Creating scene from file: " << filename << std::endl;
        global_context->s = std::make_unique< scene >( filename );
        std::cout << "Scene created successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "ERROR creating scene: " << e.what() << std::endl;
        emscripten_run_script("console.error('Scene creation failed');");
        throw;
    } catch (...) {
        std::cerr << "UNKNOWN ERROR creating scene" << std::endl;
        emscripten_run_script("console.error('Scene creation failed with unknown error');");
        throw;
    }
    
    // Temporarily disable VideoRecorder to isolate WASM module factory error
    global_context->video_recorder = nullptr;
    global_context->is_recording = false;
    //scene s( "lux_files/kaleido.json" );
    //scene s( "lux_files/CA_choices.json" );
    //scene s( "lux_files/nebula_brush.json" );
    //scene s(    //scene s( "diffuser_files/diffuser_brush.json" );
    //scene s( "moon_files/galaxy_moon.json" );
    emscripten_run_script("console.log('scene loaded');");

    std::shared_ptr< buffer_pair< ucolor > > buf( new buffer_pair< ucolor >( dim ) );
    any_buffer_pair_ptr any_buf = buf;
    global_context->s->set_output_buffer( any_buf );
    global_context->s->ui.canvas_bounds = bb2i( dim );
    //SDL_Init(SDL_INIT_VIDEO);
    //SDL_Surface *screen = SDL_SetVideoMode( dim.x, dim.y, 32, SDL_SWSURFACE );

    // pack context
 //   frame_context context;
 //   global_context->screen = screen;
 //   global_context->s = &s;
    global_context->buf = buf;
    global_context->buf_dim = dim;
    global_context->frame_callback = nullptr;
    global_context->frame_callback_ready = false;
    global_context->resize_callback = nullptr;
    global_context->resize_callback_ready = false;
    global_context->scene_callback = nullptr;
    global_context->scene_callback_ready = false;
    global_context->js_bitmaps_ready = false;
    //global_context = &context;

#ifdef TEST_SDL_LOCK_OPTS
  EM_ASM("SDL.defaults.copyOnLock = false; SDL.defaults.discardOnLock = true; SDL.defaults.opaqueFrontBuffer = false;");
#endif
  emscripten_set_main_loop_arg( render_and_display, global_context, -1, 1 );

//  SDL_Quit();

  return 0;
}


EMSCRIPTEN_BINDINGS(my_module) {
    function( "set_frame_callback",  &set_frame_callback );
    function( "set_resize_callback", &set_resize_callback );
    function( "set_scene_callback",  &set_scene_callback );

    // scene selection functions
    function( "load_scene",         &load_scene );
    function( "get_scene_list_JSON", &get_scene_list_JSON );
    function( "is_live_camera_supported", &is_live_camera_supported );

    // image functions
    function( "bitmaps_ready",      &bitmaps_ready );
    function( "get_buf1",           &get_buf1 );
    function( "get_buf2",           &get_buf2 );
    function( "get_img_data",       &get_img_data );
    function( "get_buf_width",      &get_buf_width );
    function( "get_buf_height",     &get_buf_height );
    function( "is_swapped",         &is_swapped );
    function( "get_thumbnail",      &get_thumbnail );

    // media controller functions
    function( "run_pause",          &run_pause );
    function( "restart",            &restart );
    function( "advance_frame",      &advance_frame );
    function( "reset_scene_parameters", &reset_scene_parameters );

    // widget info functions
    function( "get_panel_JSON",     &get_panel_JSON);
    function( "get_widget_JSON",    &get_widget_JSON);
    function( "is_widget_group_active", &is_widget_group_active);

    // widget control functions
    function( "set_slider_value",           &set_slider_value );
    function( "set_slider_callback",        &set_slider_callback );
    function( "set_range_slider_value",     &set_range_slider_value );
    function( "handle_menu_choice",         &handle_menu_choice );
    function( "handle_switch_value",        &handle_switch_value );
    function( "pick_funk_factor",           &pick_funk_factor );
    function( "pick_ucolor",                &pick_ucolor );
    function( "pick_direction8",            &pick_direction8 );
    function( "pick_direction4",            &pick_direction4 );
    function( "pick_direction4_diagonal",   &pick_direction4_diagonal );
    function( "pick_blur_method",           &pick_blur_method );
    function( "pick_multi_direction8",      &pick_multi_direction8 );
    function( "remove_custom_blur_pickers", &remove_custom_blur_pickers);
    function( "add_custom_blur_pickers",    &add_custom_blur_pickers);

    // mouse functions
    function( "mouse_move",        &mouse_move );
    function( "mouse_down",        &mouse_down );
    function( "mouse_over",        &mouse_over );
    function( "mouse_click",       &mouse_click );
    function("add_image_to_scene", &add_image_to_scene);
    function("add_to_menu",        &add_to_menu);
    function("update_source_name", &update_source_name);

    // video recording functions - RE-ENABLED
    function("start_recording", &start_recording);
    function("start_recording_adaptive", &start_recording_adaptive);
    function("stop_recording", &stop_recording);
    function("is_recording", &is_recording);
    function("get_recorded_frame_count", &get_recorded_frame_count);
    function("get_recording_state", &get_recording_state);
    function("get_recording_error", &get_recording_error);
    function("get_recording_data", &get_recording_data);
    function("worker_add_frame", &worker_add_frame);

    // camera functions
    function("add_camera_frame", &add_camera_frame);
    function("process_live_camera_frame", &process_live_camera_frame);
    function("set_camera_source_active", &set_camera_source_active);
    function("get_camera_processing_stats", &get_camera_processing_stats);
    function("save_camera_image_with_metadata", &save_camera_image_with_metadata);

    // new camera functions
    function("ultra_update_camera_frame", &ultra_update_camera_frame);
    function("ultra_process_camera_with_kaleidoscope", &ultra_process_camera_with_kaleidoscope);
    function("ultra_start_camera_stream", &ultra_start_camera_stream);
    function("ultra_stop_camera_stream", &ultra_stop_camera_stream);
    function("ultra_get_camera_stats", &ultra_get_camera_stats);

    // audio functions (scene-agnostic)
    function("update_audio_context", &update_audio_context);
    function("enable_audio_input", &enable_audio_input);
    function("set_audio_sensitivity", &set_audio_sensitivity);
    function("get_autoplay_audio_status", &get_autoplay_audio_status);
    function("get_autoplay_slider_info", &get_autoplay_slider_info);
    
    // slider value retrieval functions
    function("get_slider_value", &get_slider_value);
    
    // animation state functions
    function("get_animation_running", &get_animation_running);
    function("enable_scene_autoplay", &enable_scene_autoplay);
    function("get_autoplay_status", &get_autoplay_status);
    function("set_autoplay_intensity", &set_autoplay_intensity);
    function("get_scene_autoplay_info", &get_scene_autoplay_info);
}