//#include <SDL.h>
#include "scene.hpp"
#include "scene_io.hpp"
#include "uimage.hpp"
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

using namespace emscripten;

//const val document = val::global("document");

static bool running = true;
static bool displayed = false;

// used for stuffing everything needed to iterate and display a frame into a single void*
struct frame_context {
  //SDL_Surface *screen;
  std::function<void()> frame_callback;
  bool callback_ready;
  scene *s;
  std::shared_ptr< buffer_pair< ucolor > > buf;
};

frame_context *global_context;

val get_img_data() {
    uimage& img = (uimage &)(global_context->buf->get_image());
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

void set_frame_callback(val callback) {
    global_context->frame_callback = [callback]() mutable {
        callback();
    };
    global_context->callback_ready = true;
}

// used as emscripten main loop
void render_and_display( void *arg )
{
    //emscripten_run_script("console.log('render and display');");
    if( global_context->callback_ready ) {
        //emscripten_run_script("console.log('callback ready');");

        if( !running && displayed ) {
            global_context->s->ui.mouse_click = false;
            return;
        }
        
        global_context->frame_callback();
        
        if( running || !displayed ) {
            global_context->s->render();
        }

        global_context->s->ui.mouse_click = false;
        displayed = true;
    }
    else {
        emscripten_run_script("console.log('callback not ready');");
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

void mouse_move( float x, float y ) {
    global_context->s->ui.mouse_pixel = vec2i( { x, y } );
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

// slider value set to range [0.0, 1.0]
void slider_value( int value ) {
    emscripten_run_script("console.log('slider_value called');");
    std::cout << "slider value: " << value << std::endl;
    //global_context->s->ui.slider_value = ( float )std::stoi( value ) / 100.0f;
    global_context->s->ui.slider_value = value / 100.0f;
}

int main(int argc, char** argv) {
    vec2i dim( { 512, 512 } );
    //auto dims = img.get_dim();
    emscripten_run_script("console.log('preparing to load scene');");
    scene s( "nebula_files/nebula_brush.json" ); 
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
    context.callback_ready = false;
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
    function( "get_img_data",       &get_img_data);
    function( "get_buf_width",      &get_buf_width );
    function( "get_buf_height",     &get_buf_height );
    function( "run_pause",    &run_pause );
    function( "restart",      &restart );
    function( "slider_value", &slider_value );
    function( "mouse_move",   &mouse_move );
    function( "mouse_down",   &mouse_down );
    function( "mouse_over",   &mouse_over );
    function( "mouse_click",  &mouse_click );
}