
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

using namespace emscripten;

//const val document = val::global("document");

static bool running = true;
static bool displayed = false;
static bool advance = false;

// used for stuffing everything needed to iterate and display a frame into a single void*
struct frame_context {
  //SDL_Surface *screen;
  std::function<void()> frame_callback, update_callback;
  bool frame_callback_ready, update_callback_ready, js_bitmaps_ready;
  scene *s;
  std::shared_ptr< buffer_pair< ucolor > > buf;
};

frame_context *global_context;

val get_buf1() {
    uimage& img = (uimage &)(global_context->buf->get_image());
    unsigned char* buffer = (unsigned char* )img.get_base();
    size_t buffer_length = img.get_dim().x * img.get_dim().y * 4; // Assuming 4 bytes per pixel (RGBA)

    //std::cout << "get_img_data() buffer length: " << buffer_length << std::endl;
    // Create a typed memory view at the specified memory location.
    return val(typed_memory_view(buffer_length, buffer));
}

val get_img_data() { return get_buf1(); } 

val get_buf2() {
    uimage& img = (uimage &)(global_context->buf->get_buffer());
    unsigned char* buffer = (unsigned char* )img.get_base();
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

void set_frame_callback(val callback) {
    global_context->frame_callback = [callback]() mutable {
        callback();
    };
    global_context->frame_callback_ready = true; 
}

void set_update_callback(val callback) {
    global_context->update_callback = [callback]() mutable {
        callback();
    };
    global_context->update_callback_ready = true;
}

void bitmaps_ready() {
    global_context->js_bitmaps_ready = true;
}

// used as emscripten main loop
void render_and_display( void *arg )
{
    //emscripten_run_script("console.log('render and display');");
    if( global_context->frame_callback_ready ) {
        //emscripten_run_script("console.log('callback and bitmaps ready');");

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
    running = !running;
}

void restart() {
    //global_context->frame = 0;
    global_context->s->restart();
    displayed = false;
}

void advance_frame() {
    advance = true;
    running = false;
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
    if( global_context->s->float_fns.contains( name ) ) {
        std::get< std::shared_ptr< slider_float > >(global_context->s->float_fns[ name ].any_float_fn)->value = value;
    }
    else if( global_context->s->int_fns.contains( name ) ) {
        std::get< std::shared_ptr< slider_int > >(global_context->s->int_fns[ name ].any_int_fn)->value = (int)std::roundf( value );
    }
}

void set_range_slider_value( std::string name, float value_min, float value_max ) {
    if( global_context->s->interval_float_fns.contains( name ) ) {
        std::get< std::shared_ptr< range_slider_float > >(global_context->s->interval_float_fns[ name ].any_interval_float_fn)->value = interval_float( value_min, value_max );
    }
    else if( global_context->s->interval_int_fns.contains( name ) ) {
        std::get< std::shared_ptr< range_slider_int > >(global_context->s->interval_int_fns[ name ].any_interval_int_fn)->value = interval_int( (int)std::roundf( value_min ), (int)std::roundf( value_max ) );
    }
}
/*
// instead of all these calls, just return a piece of JSON that contains all the information needed to build the UI
float get_slider_min( std::string name ) {
    if( global_context->s->float_fns.contains( name ) ) {
        return std::get< std::shared_ptr< slider_float > >(global_context->s->float_fns[ name ].any_float_fn)->min;
    }
    else if( global_context->s->int_fns.contains( name ) ) {
        return (float)std::get< std::shared_ptr< slider_int > >(global_context->s->int_fns[ name ].any_int_fn)->min;
    }
    else if( global_context->s->interval_float_fns.contains( name ) ) {
        return std::get< std::shared_ptr< range_slider_float > >(global_context->s->interval_float_fns[ name ].any_interval_float_fn)->value.min;
    }
    else if( global_context->s->interval_int_fns.contains( name ) ) {
        return (float)std::get< std::shared_ptr< range_slider_int > >(global_context->s->interval_int_fns[ name ].any_interval_int_fn)->value.min;
    }
    else return 0.0f;
}

float get_slider_max( std::string name ) {
    if( global_context->s->float_fns.contains( name ) ) {
        return std::get< std::shared_ptr< slider_float > >(global_context->s->float_fns[ name ].any_float_fn)->max;
    }
    else if( global_context->s->int_fns.contains( name ) ) {
        return (float)std::get< std::shared_ptr< slider_int > >(global_context->s->int_fns[ name ].any_int_fn)->max;
    }
    else if( global_context->s->interval_float_fns.contains( name ) ) {
        return std::get< std::shared_ptr< range_slider_float > >(global_context->s->interval_float_fns[ name ].any_interval_float_fn)->value.max;
    }
    else if( global_context->s->interval_int_fns.contains( name ) ) {
        return (float)std::get< std::shared_ptr< range_slider_int > >(global_context->s->interval_int_fns[ name ].any_interval_int_fn)->value.max;
    }
    else return 0.0f;
}

float get_slider_value( std::string name ) {
    if( global_context->s->float_fns.contains( name ) ) {
        return std::get< std::shared_ptr< slider_float > >(global_context->s->float_fns[ name ].any_float_fn)->value;
    }
    else if( global_context->s->int_fns.contains( name ) ) {
        return (float)std::get< std::shared_ptr< slider_int > >(global_context->s->int_fns[ name ].any_int_fn)->value;
    }
    else return 0.0f;
}

interval_float get_range_slider_value( std::string name ) {
    if( global_context->s->interval_float_fns.contains( name ) ) {
        return std::get< std::shared_ptr< range_slider_float > >(global_context->s->interval_float_fns[ name ].any_interval_float_fn)->value;
    }
    else if( global_context->s->interval_int_fns.contains( name ) ) {
        interval_int i = std::get< std::shared_ptr< range_slider_int > >(global_context->s->interval_int_fns[ name ].any_interval_int_fn)->value;
        return interval_float( (float)i.min, (float)i.max );
    }
    else return interval_float( 0.0f, 0.0f );
}

float get_slider_step( std::string name ) {
    if( global_context->s->float_fns.contains( name ) ) {
        return std::get< std::shared_ptr< slider_float > >(global_context->s->float_fns[ name ].any_float_fn)->step;
    }
    else if( global_context->s->int_fns.contains( name ) ) {
        return (float)std::get< std::shared_ptr< slider_int > >(global_context->s->int_fns[ name ].any_int_fn)->step;
    }
    else if( global_context->s->interval_float_fns.contains( name ) ) {
        return std::get< std::shared_ptr< range_slider_float > >(global_context->s->interval_float_fns[ name ].any_interval_float_fn)->step;
    }
    else if( global_context->s->interval_int_fns.contains( name ) ) {
        return (float)std::get< std::shared_ptr< range_slider_int > >(global_context->s->interval_int_fns[ name ].any_interval_int_fn)->step;
    }
    else return 0.0f;
}

std::string get_slider_label( std::string name ) {
    if( global_context->s->float_fns.contains( name ) ) {
        return std::get< std::shared_ptr< slider_float > >(global_context->s->float_fns[ name ].any_float_fn)->label;
    }
    else if( global_context->s->int_fns.contains( name ) ) {
        return std::get< std::shared_ptr< slider_int > >(global_context->s->int_fns[ name ].any_int_fn)->label;
    }
    else if( global_context->s->interval_float_fns.contains( name ) ) {
        return std::get< std::shared_ptr< range_slider_float > >(global_context->s->interval_float_fns[ name ].any_interval_float_fn)->label;
    }
    else if( global_context->s->interval_int_fns.contains( name ) ) {
        return std::get< std::shared_ptr< range_slider_int > >(global_context->s->interval_int_fns[ name ].any_interval_int_fn)->label;
    }
    else return "Slider";
}

std::string get_slider_description( std::string name ) {
    if( global_context->s->float_fns.contains( name ) ) {
        return std::get< std::shared_ptr< slider_float > >(global_context->s->float_fns[ name ].any_float_fn)->description;
    }
    else if( global_context->s->int_fns.contains( name ) ) {
        return std::get< std::shared_ptr< slider_int > >(global_context->s->int_fns[ name ].any_int_fn)->description;
    }
    else if( global_context->s->interval_float_fns.contains( name ) ) {
        return std::get< std::shared_ptr< range_slider_float > >(global_context->s->interval_float_fns[ name ].any_interval_float_fn)->description;
    }
    else if( global_context->s->interval_int_fns.contains( name ) ) {
        return std::get< std::shared_ptr< range_slider_int > >(global_context->s->interval_int_fns[ name ].any_interval_int_fn)->description;
    }
    else return "Slider description";
}

std::string get_menu_choices( std::string name ) {
    std::vector< std::string > items;
    if( global_context->s->int_fns.contains( name ) ) {
        auto& menu = std::get< std::shared_ptr< menu_int > >(global_context->s->int_fns[ name ].any_int_fn);
        items = menu->items;
    }
    else if( global_context->s->string_fns.contains( name ) ) {
        auto& menu = std::get< std::shared_ptr< menu_string > >(global_context->s->string_fns[ name ].any_string_fn);
        items = menu->items;
    }
    else {
        std::cout << "get_menu_choices: Rule chooser not found" << std::endl;
        return "Choice 1,Choice 2,Choice 3";
    }
    std::ostringstream oss;
    for (const auto &item : items) {
        if (item != items[0]) {
            oss << ",";
        }
        oss << item;
    }
    std::cout << "get_menu_choices: " << oss.str() << std::endl;
    return oss.str();
}

int get_default_menu_choice( std::string name ) {
    if( global_context->s->int_fns.contains( name ) ) {
        auto& menu = std::get< std::shared_ptr< menu_int > >(global_context->s->int_fns[ name ].any_int_fn);
        return menu->choice;
    }
    else if( global_context->s->string_fns.contains( name ) ) {
        auto& menu = std::get< std::shared_ptr< menu_string > >(global_context->s->string_fns[ name ].any_string_fn);
        return menu->choice;
    }
    else {
        return 0;
    }
}

std::string get_menu_label( std::string name ) {
    if( global_context->s->int_fns.contains( name ) ) {
        auto& menu = std::get< std::shared_ptr< menu_int > >(global_context->s->int_fns[ name ].any_int_fn);
        return menu->label;
    }
    else if( global_context->s->string_fns.contains( name ) ) {
        auto& menu = std::get< std::shared_ptr< menu_string > >(global_context->s->string_fns[ name ].any_string_fn);
        return menu->label;
    }
    else {
        return "Menu";
    }
}

std::string get_menu_description( std::string name ) {
    if( global_context->s->int_fns.contains( name ) ) {
        auto& menu = std::get< std::shared_ptr< menu_int > >(global_context->s->int_fns[ name ].any_int_fn);
        return menu->description;
    }
    else if( global_context->s->string_fns.contains( name ) ) {
        auto& menu = std::get< std::shared_ptr< menu_string > >(global_context->s->string_fns[ name ].any_string_fn);
        return menu->description;
    }
    else {
        return "Menu description";
    }
} */

void handle_menu_choice( std::string name, int choice ) {
    if( global_context->s->int_fns.contains( name ) ) {
        std::cout << "handle_menu_choice (menu_int): " << name << " " << choice << std::endl;
        auto& menu = std::get< std::shared_ptr< menu_int > >(global_context->s->int_fns[ name ].any_int_fn);
        menu->choose( choice );
    }
    else if( global_context->s->string_fns.contains( name ) ) {
        std::cout << "handle_menu_choice (menu_string): " << name << " " << choice << std::endl;
        auto& menu = std::get< std::shared_ptr< menu_string > >(global_context->s->string_fns[ name ].any_string_fn);
        menu->choose( choice );
    }
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

std::string get_panel_JSON() {
    std::string fileContents;

    fileContents = load_file_as_string("nebula_files/test_widgets.json");
    return fileContents;
}


int main(int argc, char** argv) { 
    vec2i dim( { 512, 512 } );  // original sin
    //auto dims = img.get_dim();
    emscripten_run_script("console.log('preparing to load scene');");
    scene s( "nebula_files/CA_choices.json" ); 
    //scene s( "nebula_files/nebula_brush.json" ); 
    //scene s(    //scene s( "diffuser_files/diffuser_brush.json" ); 
    //scene s( "moon_files/galaxy_moon.json" );
    emscripten_run_script("console.log('scene loaded');");

    std::shared_ptr< buffer_pair< ucolor > > buf( new buffer_pair< ucolor >( dim ) );
    any_buffer_pair_ptr any_buf = buf;
    s.set_output_buffer( any_buf );
    s.ui.canvas_bounds = bb2i( dim );
    //SDL_Init(SDL_INIT_VIDEO); 
    //SDL_Surface *screen = SDL_SetVideoMode( dim.x, dim.y, 32, SDL_SWSURFACE );

    // pack context
    frame_context context;
 //   context.screen = screen;
    context.s = &s;
    context.buf = buf;
    context.frame_callback = nullptr;
    context.frame_callback_ready = false;
    context.update_callback = nullptr;
    context.update_callback_ready = false;
    context.js_bitmaps_ready = false;
    global_context = &context;

#ifdef TEST_SDL_LOCK_OPTS
  EM_ASM("SDL.defaults.copyOnLock = false; SDL.defaults.discardOnLock = true; SDL.defaults.opaqueFrontBuffer = false;");
#endif
  emscripten_set_main_loop_arg( render_and_display, &context, -1, 1 );

//  SDL_Quit();

  return 0;
}
 
EMSCRIPTEN_BINDINGS(my_module) {
    function( "set_frame_callback", &set_frame_callback );
    function( "set_update_callback",&set_update_callback );

    function( "bitmaps_ready",      &bitmaps_ready );
    function( "get_buf1",           &get_buf1 );
    function( "get_buf2",           &get_buf2 );
    function( "get_img_data",       &get_img_data );
    function( "get_buf_width",      &get_buf_width );
    function( "get_buf_height",     &get_buf_height );
    function( "is_swapped",         &is_swapped );

    function( "run_pause",          &run_pause );
    function( "restart",            &restart );
    function( "advance_frame",      &advance_frame );

    function( "get_panel_JSON",     &get_panel_JSON);

    function( "set_slider_value",       &set_slider_value );
    function( "set_range_slider_value", &set_range_slider_value );
    function( "handle_menu_choice",     &handle_menu_choice );

    /*
    function( "get_slider_min",     &get_slider_min);
    function( "get_slider_max",     &get_slider_max);
    function( "get_slider_value",   &get_slider_value);
    function( "get_slider_step",    &get_slider_step);
    function( "get_slider_label",   &get_slider_label);
    function( "get_slider_description", &get_slider_description);

    function( "get_menu_choices",   &get_menu_choices);
    function( "get_default_menu_choice", &get_default_menu_choice);
    function( "get_menu_label",     &get_menu_label);
    function( "get_menu_description", &get_menu_description); 
    */
    
    function( "mouse_move",         &mouse_move );
    function( "mouse_down",         &mouse_down );
    function( "mouse_over",         &mouse_over );
    function( "mouse_click",        &mouse_click );
}
 