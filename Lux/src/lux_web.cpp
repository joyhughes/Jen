
//#include <SDL.h>
#include "scene.hpp"
#include "scene_io.hpp"
#include "uimage.hpp"
#include "effect.hpp"
#include "any_effect.hpp"
#include "life.hpp"
#include "any_rule.hpp"
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

void slider_value( float value ) {
    std::cout << "slider_value: " << value << std::endl;
    global_context->s->ui.main_slider.value = value;
}

float get_slider_min( std::string name ) {
    std::cout << "get_slider_min: " << global_context->s->ui.main_slider.min << std::endl;
    return global_context->s->ui.main_slider.min;
}

float get_slider_max( std::string name ) {
    std::cout << "get_slider_max: " << global_context->s->ui.main_slider.max << std::endl;
    return global_context->s->ui.main_slider.max;
}

float get_slider_value( std::string name ) {
    return global_context->s->ui.main_slider.value;
}

float get_slider_step( std::string name ) {
    return global_context->s->ui.main_slider.step;
}

std::string get_slider_label( std::string name ) {
    return global_context->s->ui.main_slider.label;
}

std::string get_slider_description( std::string name ) {
    return "Sort Threshold is the maximum brightness difference between two pixels that will be sorted.";
    //return global_context->s->ui.main_slider.description;
}

std::string get_menu_choices( std::string name ) {
    if( global_context->s->effects.contains( "Rule Chooser" ) ) {
        auto& eff = std::get< std::shared_ptr< eff_chooser > >(global_context->s->effects[ "Rule Chooser" ].fn_ptr);
        std::ostringstream oss;
        for (const auto &choice : eff->effects) {
            if (&choice.name != &eff->effects[0].name) {
                oss << ",";
            }
            oss << choice.name;
        }
        std::cout << "get_menu_choices: " << oss.str() << std::endl;
        return oss.str();
    }
    else {
        std::cout << "get_menu_choices: Rule chooser not found" << std::endl;
        return "Choice 1,Choice 2,Choice 3";
    }
}

int get_initial_menu_choice( std::string name ) {

    if( global_context->s->effects.contains( "Rule Chooser" ) ) {
        auto& eff = std::get< std::shared_ptr< eff_chooser > >(global_context->s->effects[ "Rule Chooser" ].fn_ptr);
        std::cout << "get_initial_menu_choice: " << *(eff->choice) << std::endl;
        return *(eff->choice);
    }
    else {
        std::cout << "get_initial_menu_choice: Rule chooser not found" << std::endl;
        return 2;
    }
}

std::string get_menu_label( std::string name ) {
    return "Transition Rule";
    /*
    if( global_context->s->effects.contains( "Rule Chooser" ) ) {
        auto& eff = std::get< std::shared_ptr< eff_chooser > >(global_context->s->effects[ "Rule Chooser" ].fn_ptr);
        std::cout << "get_menu_label: " << eff->label << std::endl;
        return eff->label;
    }
    else {
        std::cout << "get_menu_label: Rule chooser not found" << std::endl;
        return "Rule Chooser";
    }
    */
}

std::string get_menu_description( std::string name ) {
    return "Transition rules include cellular automata such as Conway's Game of Life, and other ways each frame can depend on the previous one.";
    /*
    if( global_context->s->effects.contains( "Rule Chooser" ) ) {
        auto& eff = std::get< std::shared_ptr< eff_chooser > >(global_context->s->effects[ "Rule Chooser" ].fn_ptr);
        std::cout << "get_menu_description: " << eff->description << std::endl;
        return eff->description;
    }
    else {
        std::cout << "get_menu_description: Rule chooser not found" << std::endl;
        return "Choose a transition rule";
    }*/
}
void handle_menu_choice( std::string name, int choice ) {
    std::cout << "handle_menu_choice: " << choice << std::endl;
    auto& eff = std::get< std::shared_ptr< eff_chooser > >(global_context->s->effects[ "Rule Chooser" ].fn_ptr);
    eff->choose( choice );
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
    context.s->ui.main_slider = slider( "Sort Threshold", 200.0f, 0.0f, 766.0f, 1.0f );
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

    function( "slider_value",       &slider_value );
    function( "get_slider_min",     &get_slider_min);
    function( "get_slider_max",     &get_slider_max);
    function( "get_slider_value",   &get_slider_value);
    function( "get_slider_step",    &get_slider_step);
    function( "get_slider_label",   &get_slider_label);
    function( "get_slider_description", &get_slider_description);

    function( "get_menu_choices",   &get_menu_choices);
    function( "get_initial_menu_choice",   &get_initial_menu_choice);
    function( "get_menu_label",     &get_menu_label);
    function( "get_menu_description", &get_menu_description);
    function( "handle_menu_choice", &handle_menu_choice);
    
    function( "mouse_move",         &mouse_move );
    function( "mouse_down",         &mouse_down );
    function( "mouse_over",         &mouse_over );
    function( "mouse_click",        &mouse_click );
}
 