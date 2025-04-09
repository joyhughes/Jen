
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
//#include <chrono>
//#include <thread>

using namespace emscripten;
#define DEBUG( msg ) { std::string debug_msg = msg; std::cout << debug_msg << std::endl; }
#define ERROR( msg ) throw std::runtime_error( msg );


// used for stuffing everything needed to iterate and display a frame into a single void*
struct frame_context {
  //SDL_Surface *screen;
  std::function<void()> frame_callback, resize_callback, scene_callback;
  bool frame_callback_ready, resize_callback_ready, scene_callback_ready, js_bitmaps_ready;
  std::unique_ptr< scene > s;
  std::shared_ptr< buffer_pair< ucolor > > buf;
  vec2i buf_dim;
  nlohmann::json scene_list;
};

frame_context *global_context;

val get_buf1() {
    uimage& img = (uimage &)(global_context->buf->get_image());
    unsigned char* buffer = (unsigned char* )img.get_base_ptr();
    size_t buffer_length = img.get_dim().x * img.get_dim().y * 4; // Assuming 4 bytes per pixel (RGBA)

    //std::cout << "get_img_data() buffer length: " << buffer_length << std::endl;
    // Create a typed memory view at the specified memory location.
    return val(typed_memory_view(buffer_length, buffer));
} 

val get_img_data() { return get_buf1(); } 

val get_buf2() {
    uimage& img = (uimage &)(global_context->buf->get_buffer());
    unsigned char* buffer = (unsigned char* )img.get_base_ptr();
    size_t buffer_length = img.get_dim().x * img.get_dim().y * 4; // Assuming 4 bytes per pixel (RGBA)

    //std::cout << "get_img_data() buffer length: " << buffer_length << std::endl;
    // Create a typed memory view at the specified memory location.
    return val(typed_memory_view(buffer_length, buffer));
}

int get_buf_width() {
    uimage& img = (uimage &)(global_context->buf->get_image());
    return img.get_dim().x;
}

int get_buf_height() {
    uimage& img = (uimage &)(global_context->buf->get_image());
    return img.get_dim().y;
}

bool is_swapped() {
    return global_context->buf->is_swapped();
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
    // IMPORTANT: The JS side MUST be finished with this view before this C++ function scope ends,
    // otherwise 'buffer' will point to deallocated memory from thumb_img_ptr going out of scope.
    // For simple drawing, this is usually okay as JS copies it quickly.
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

        if( !running && !advance && displayed ) {
            global_context->s->ui.mouse_click = false;
            return;
        }
        
        global_context->frame_callback();
        
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

void advance_frame() {
    global_context->s->ui.advance = true;
    global_context->s->ui.running = false;
}

void mouse_move( int x, int y, int width, int height ) {
    UI& ui = global_context->s->ui;
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
    std::cout << "C++ set_slider_value: Received name='" << name << "', value=" << value << std::endl;
    if( global_context->s->functions.contains( name ) ) {
        any_function& fn = global_context->s->functions[ name ];
        if( std::holds_alternative< any_fn< float > >( fn ) ) {
            global_context->s->get_fn_ptr< float, slider_float >( name )->value = value;
            return;
        }
        else if( std::holds_alternative< any_fn< int > >( fn ) ) {
           global_context->s->get_fn_ptr< int, slider_int >( name )->value = (int)std::roundf( value );
           return;
        }
        throw std::runtime_error( "slider " + name + " invalid type " );
    }
    std::cerr << "  Slider '" << name << "' not found or invalid type in set_slider_value." << std::endl;
    throw std::runtime_error( "slider " + name + " not found in scene" );
}

void set_range_slider_value( std::string name, float value_min, float value_max ) {
    // std::cout << "set_range_slider_value: " << name << " " << value_min << " " << value_max << std::endl;
    if( global_context->s->functions.contains( name ) ) { 
        any_function& fn = global_context->s->functions[ name ];
        if( std::holds_alternative< any_fn< interval_float > >( fn ) ) {
            global_context->s->get_fn_ptr< interval_float, range_slider_float >( name )->value = interval_float( value_min, value_max );
            return;
        }
        else if( std::holds_alternative< any_fn< interval_int > >( fn ) ) {
            //std::cout << "range_slider_int: " << name << " " << value_min << " " << value_max << std::endl;
            global_context->s->get_fn_ptr< interval_int, range_slider_int >( name )->value = interval_int( (int)std::roundf( value_min ), (int)std::roundf( value_max ) );
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

/*
bool get_switch_state( std::string name ) {
    if( global_context->s->functions.contains( name ) ) {
        auto& sw = std::get< std::shared_ptr< switch_fn > >( std::get< any_fn< bool > >( global_context->s->functions[ name ] ).any_bool_fn);
        return sw->value;
    }
    else if( global_context->s->functions.contains( name ) ) {
        auto& sw = std::get< std::shared_ptr< switch_condition > >(global_context->s->condition_fns[ name ].my_condition_fn);
        return sw->value;
    }
    return false;
}
*/

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


void add_image_to_scene(std::string name, int width, int height, emscripten::val pixelDataVal) {
    // Use DEBUG/ERROR macros defined in scene_io.hpp or similar
    DEBUG("C++ add_image_to_scene: Adding '" + name + "' (" + std::to_string(width) + "x" + std::to_string(height) + ")");

    if (!global_context || !global_context->s) {
        std::cerr << "ERROR: Global context or scene not available in add_image_to_scene." << std::endl;
        // Cannot use ERROR macro here if it relies on global_context->s
        return; // Early exit if context is missing
    }

    // 1. Check for name collision (Optional: Decide on overwrite or error)
    if (global_context->s->buffers.count(name)) {
        std::cerr << "Warning: Image name '" + name + "' already exists. Overwriting." << std::endl;
    }

    // 2. Validate dimensions
    if (width <= 0 || height <= 0) {
        ERROR("Invalid dimensions for image '" + name + "'"); // Calls exit(0) via emscripten_error
        return; // Return added for clarity, though ERROR likely halts
    }

    // 3. Create new uimage (using unique_ptr for safety)
    std::unique_ptr<uimage> new_image;
    try {
       new_image = std::make_unique<uimage>(vec2i{width, height});
    } catch (const std::bad_alloc& e) {
       ERROR("ERROR: Failed to allocate memory for uimage '" + name + "': " + std::string(e.what()));
       return;
    } catch (...) {
       ERROR("ERROR: Unknown error creating uimage for '" + name + "'");
       return;
    }

    if (!new_image) { // Should be caught by exceptions, but belts-and-suspenders
        ERROR("ERROR: Failed to create new image buffer (make_unique returned null?) for '" + name + "'");
        return;
    }

    // 4. Get pixel data pointer and length from JS val
    size_t expected_length = static_cast<size_t>(width) * height * 4; // RGBA
    size_t actual_length = 0;
    uintptr_t data_ptr_addr = 0;
    const unsigned char* js_pixels_ptr = nullptr;

    try {
        // Verify it looks like a TypedArray view before accessing properties
        bool is_valid_typed_array = !pixelDataVal["byteLength"].isUndefined() &&
                                    !pixelDataVal["byteOffset"].isUndefined() &&
                                    !pixelDataVal["buffer"].isUndefined() &&
                                     pixelDataVal["buffer"]["byteLength"].isNumber(); // Check buffer exists and has length

        if (!is_valid_typed_array) {
             ERROR("Passed pixelDataVal is not a valid JS TypedArray/DataView for image '" + name + "'");
             return;
        }

        actual_length = pixelDataVal["byteLength"].as<size_t>();
        if (actual_length != expected_length) {
            ERROR("Pixel data size mismatch for '" + name + "'. Expected=" + std::to_string(expected_length) + ", Got=" + std::to_string(actual_length));
            return;
        }

        // Get the numeric address of the start of the data view
        // Access ArrayBuffer address via ['buffer']['byteLength'] as a trick to get its address as number
        data_ptr_addr = pixelDataVal["buffer"]["byteLength"].as<uintptr_t>() - pixelDataVal["buffer"]["byteLength"].as<size_t>() ; // Get base address of ArrayBuffer
        size_t offset = pixelDataVal["byteOffset"].as<size_t>(); // Get the view's offset within the buffer
        data_ptr_addr += offset; // Add offset to get the start of this view's data

        // Cast numeric address back to C++ pointer (only used locally)
        js_pixels_ptr = reinterpret_cast<const unsigned char*>(data_ptr_addr);

    } catch (const std::exception& e) {
         // Catch potential errors during property access or .as<T>() conversions
         ERROR("Exception accessing pixelDataVal properties for image '" + name + "': " + std::string(e.what()));
         return;
    } catch (...) {
         ERROR("Unknown exception accessing pixelDataVal for image '" + name + "'");
         return;
    }

    // 5. Copy and potentially reorder pixels
    unsigned char* cpp_buffer = (unsigned char*)new_image->get_base_ptr();
    DEBUG("  Starting pixel copy for '" + name + "'...");
    try {
        // Assuming JS ImageData (pixelDataVal) is RGBA
        // Assuming C++ ucolor is ARGB (0xAARRGGBB)
        // *** VERIFY YOUR ACTUAL UCOLOR LAYOUT ***
        for(size_t i = 0; i < expected_length; i+= 4) {
            const unsigned char* js_pixel = js_pixels_ptr + i; // Pointer to start of RGBA pixel
                  unsigned char* cpp_pixel_loc = cpp_buffer + i; // Pointer to destination ARGB pixel location

            // Read RGBA
            unsigned char r = js_pixel[0];
            unsigned char g = js_pixel[1];
            unsigned char b = js_pixel[2];
            unsigned char a = js_pixel[3];

            // Write ARGB (adjust byte order if ucolor is different, e.g., ABGR)
            cpp_pixel_loc[0] = a; // Alpha
            cpp_pixel_loc[1] = r; // Red
            cpp_pixel_loc[2] = g; // Green
            cpp_pixel_loc[3] = b; // Blue

            // If ucolor is stored as uint32_t:
            // ucolor pixel = (static_cast<ucolor>(a) << 24) | (static_cast<ucolor>(r) << 16) |
            //                (static_cast<ucolor>(g) << 8)  | (static_cast<ucolor>(b));
            // memcpy(cpp_buffer + (i / 4) * sizeof(ucolor), &pixel, sizeof(ucolor));
        }
        // If ucolor *is* RGBA (same as JS ImageData):
        // memcpy(cpp_buffer, js_pixels_ptr, expected_length);

    } catch (const std::exception& e) {
        ERROR("Exception during pixel copy for '" + name + "': " + std::string(e.what()));
        return;
    } catch (...) {
        ERROR("Unknown exception during pixel copy for '" + name + "'");
        return;
    }
    DEBUG("  Pixel copy finished for '" + name + "'.");


    // 6. Create buffer pair, adopt image, initialize mipmaps
    ubuf_ptr new_buffer_pair = std::make_shared<buffer_pair<ucolor>>();
    new_buffer_pair->adopt_image(std::move(new_image)); // Transfer ownership
    new_buffer_pair->initialize(); // Generate mipmaps

    // 7. Add to scene buffers map (overwrites if key exists)
    global_context->s->buffers[name] = new_buffer_pair;
    DEBUG("  Successfully added buffer '" + name + "' to scene map.");

    // 8. Call JS function to update the UI menu (Optional)
    try {
        std::string escaped_menu_name = escape_for_js_string("source_image_menu"); // Name of the JS menu state/ID
        std::string escaped_item_name = escape_for_js_string(name);
        // Construct the JS command string
        std::string js_command = "if (window.module && typeof window.module.add_to_menu === 'function') { "
                                 "  window.module.add_to_menu('" + escaped_menu_name + "', '" + escaped_item_name + "'); "
                                 "} else { console.warn('JS function window.module.add_to_menu not found or module not ready'); }";
        emscripten_run_script(js_command.c_str()); // Execute the JS command
        DEBUG("  Called JS add_to_menu('" + escaped_menu_name + "', '" + escaped_item_name + "') attempt finished.");
    } catch (...) { // Catch potential C++ exceptions during string formatting etc.
        std::cerr << "Warning: Exception occurred trying to call JS add_to_menu for '" << name << "'." << std::endl;
    }

    // 9. Restart scene to apply changes and trigger re-render
    //    (Might be possible to do a less drastic update if only source changed)
    DEBUG("  Restarting scene after adding image '" + name + "'...");
    global_context->s->restart();
    DEBUG("  add_image_to_scene completed for '" + name + "'.");
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

int main(int argc, char** argv) {
    using namespace nlohmann;
    std::string filename;
    vec2i dim( { 512, 512 } );  // original sin
    //auto dims = img.get_dim();
    emscripten_run_script("console.log('preparing to load scene');");
    global_context = new frame_context();
    global_context->scene_list = json::parse( load_file_as_string( "lux_files/scenes.json" ) );
    global_context->scene_list[ "scenes" ][ 0 ][ "filename" ].get_to( filename );
    global_context->s = std::make_unique< scene >( filename );
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

    // widget info functions
    function( "get_panel_JSON",     &get_panel_JSON);
    function( "get_widget_JSON",    &get_widget_JSON);
    function( "is_widget_group_active", &is_widget_group_active);

    // widget control functions
    function( "set_slider_value",       &set_slider_value );
    function( "set_range_slider_value", &set_range_slider_value );
    function( "handle_menu_choice",     &handle_menu_choice );
    function( "handle_switch_value",    &handle_switch_value );
//    function( "get_switch_state",       &get_switch_state);
    function( "pick_direction8",        &pick_direction8 );
    function( "pick_direction4",        &pick_direction4 );
    function( "pick_blur_method",       &pick_blur_method );
    function( "pick_multi_direction8",  &pick_multi_direction8 );
    function( "remove_custom_blur_pickers", &remove_custom_blur_pickers);
    function( "add_custom_blur_pickers", &add_custom_blur_pickers);

    // mouse functions
    function( "mouse_move",         &mouse_move );
    function( "mouse_down",         &mouse_down );
    function( "mouse_over",         &mouse_over );
    function( "mouse_click",        &mouse_click );
    function("add_image_to_scene", &add_image_to_scene);
    function("add_to_menu", &add_to_menu);
    function("update_source_name", &update_source_name);
}