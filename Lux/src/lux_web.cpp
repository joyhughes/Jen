
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

//#include <chrono>
//#include <thread>

using namespace emscripten;

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

val get_thumbnail( std::string name, int width, int height) {
    uimage img( vec2i{ width, height } );
    bool drawn = false;
    unsigned char* buffer = (unsigned char* )img.get_base_ptr();
    size_t buffer_length = width * height * 4; // Assuming 4 bytes per pixel (RGBA)

    if( global_context->s->buffers.contains( name ) ) {
        // make sure buffer contains uimage buffer pair
        if( std::holds_alternative< ubuf_ptr >( global_context->s->buffers[ name ] ) )
            img.splat( std::get< ubuf_ptr >( global_context->s->buffers[ name ] )->get_image() );
            drawn = true;
    }
    // gray out if not drawn
    if( !drawn ) {
        img.fill( 0xff080808 );
    }

    // Create a typed memory view at the specified memory location.
    return val(typed_memory_view(buffer_length, buffer));
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
    try {
        std::cout << "Updating source to: " << name << std::endl;

        if (!global_context->s->buffers.contains(name)) {
            std::cerr << "Source buffer not found: " << name << std::endl;
            return;
        }

        for (auto& q : global_context->s->queue) {
            q.rendered = false;
        }

        global_context->s->restart();

    } catch (const std::exception& e) {
        std::cerr << "Error in update_source_name: " << e.what() << std::endl;
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