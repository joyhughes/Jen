#include <SDL.h>
#include "scene.hpp"
#include "scene_io.hpp"
#include "uimage.hpp"
#include <emscripten.h>
#include <emscripten/bind.h>

using namespace emscripten;

static bool running = true;
static bool displayed = false;

// idiom for overloaded lambdas - from https://en.cppreference.com/w/cpp/utility/variant/visit
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

// used for stuffing everything needed to iterate and display a frame into a single void*
struct frame_context {
  SDL_Surface *screen;
  scene *s;
  std::shared_ptr< buffer_pair< ucolor > > buf;
  int frame, nframes;
  //int curliness;
};

frame_context *global_context;

// used as emscripten main loop
void render_and_display( void *arg )
{
    if( !running && displayed ) return;
    frame_context *context;
    context = (frame_context *)arg;
    // unpack context
    SDL_Surface *screen = context->screen;
    uimage& img = (uimage &)(context->buf->get_image());
    vec2i dim = img.get_dim();
    scene &s = *(context->s);
    float time_interval = 1.0f / (float)context->nframes;
    float time = (float)context->frame * time_interval;
    
    if (SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);

    // copy Lux buffer into SDL buffer
    unsigned int* pixel_ptr = (unsigned int*)screen->pixels;
    unsigned int* base_ptr = img.get_base();
    for( int i=0; i< dim.x * dim.y; i++ ) {
        *pixel_ptr = *base_ptr;
        pixel_ptr++; base_ptr++;
    }

    SDL_Flip(screen);
    if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);

    if( running || !displayed ) {
        // set curliness based on slider value
        /*
        float v = (float)context->curliness / 100.0f;
        any_gen_fn_ptr& fn = global_context->s->gen_fns[ "curler" ].my_gen_fn;
        std::visit (overloaded {
            [ v ]( std::shared_ptr< curly >& c ) { c->curliness = v; },
            []( auto& ) { emscripten_run_script("console.log('not a curly');"); }
        }, fn );        img.fill(0x0);
        */
        // clear image and render scene
        //any_image_ptr any_out = context->img;
        s.render();
        //displayed = false;
    }

    if( displayed ) context->frame++;
    if( context->frame >= context->nframes ) {
        context->frame = 0;
        // reset scene functors
    }
    displayed = true;
}

void run_pause() {
    running = !running;
}

void restart() {
    global_context->frame = 0;
    displayed = false;
}

void slider_value( int value ) {
    //global_context->curliness = value;
    //displayed = false;
}

/*
void slider_value( int value ) {
    float v = ( float )value / 100.0f;
    global_context->s->elements[ "moon_element" ]->position.x = v;
}
*/
int main(int argc, char** argv) {
    vec2i dim( { 512, 512 } );
    std::shared_ptr< buffer_pair< ucolor > > buf( new buffer_pair< ucolor >( dim ) );
    any_buffer_pair_ptr any_buf = buf;
    //auto dims = img.get_dim();
    emscripten_run_script("console.log('preparing to load scene');");
    scene s( "curly_files/curly.json" );

//    scene s( "samples/diffuser.json" );
    emscripten_run_script("console.log('scene loaded');");
    s.set_output_buffer( any_buf );
    //scene s( "moon_files/galaxy_moon.json" ); 
    //scene s( "foo.json" );
    //scene s;
    /*any_gen_fn_ptr& fn = s.gen_fns[ "curler" ].my_gen_fn;
    std::visit (overloaded {
        []( std::shared_ptr< curly >& c ) { c->curliness = 0.0f; },
        []( auto& ) { emscripten_run_script("console.log('not a curly');"); }
    }, fn ); */
    SDL_Init(SDL_INIT_VIDEO); 
    SDL_Surface *screen = SDL_SetVideoMode( dim.x, dim.y, 32, SDL_SWSURFACE );

    // pack context
    frame_context context;
    context.screen = screen;
    context.s = &s;
    context.buf = buf;
    context.nframes = 100;
    context.frame = 0;
    global_context = &context;

#ifdef TEST_SDL_LOCK_OPTS
  EM_ASM("SDL.defaults.copyOnLock = false; SDL.defaults.discardOnLock = true; SDL.defaults.opaqueFrontBuffer = false;");
#endif
  emscripten_set_main_loop_arg( render_and_display, &context, -1, 1 );

  SDL_Quit();

  return 0;
}

EMSCRIPTEN_BINDINGS(my_module) {
    function( "run_pause",    &run_pause );
    function( "restart",      &restart );
    function( "slider_value", &slider_value );
}
