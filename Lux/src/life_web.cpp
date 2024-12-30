#include <SDL.h>
#include "life.hpp"
#include "uimage.hpp"
#include <emscripten.h>
#include <emscripten/bind.h>

using namespace emscripten;
 
static bool running = true;
static bool displayed = false;

// used for stuffing everything needed to iterate and display a frame into a single void*
struct frame_context {
  SDL_Surface *screen;
  CA< ucolor > *my_CA;
  std::shared_ptr< pixel_sort< ucolor > > my_rule;
  image< ucolor > *img;
  buffer_pair< ucolor > *buf;
  vec2i *dims;
};

frame_context *global_context;

// used as emscripten main loop
void iterate_and_display(void *arg)
{
  if( !running && displayed ) return;
  frame_context *context;
  context = (frame_context *)arg;
  // unpack context
  SDL_Surface *screen = context->screen;
  CA< ucolor >& my_CA = *(context->my_CA);
  buffer_pair< ucolor >& buf = *(context->buf);
  vec2i dims = *(context->dims);

  if (SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);
  // copy Lux buffer into SDL buffer
  unsigned int* pixel_ptr = (unsigned int*)screen->pixels;
  unsigned int* base_ptr = buf.get_image().get_base_ptr();
  for( int i=0; i< dims.x * dims.y; i++ ) {
    *pixel_ptr = *base_ptr;
    pixel_ptr++; base_ptr++;
  }
  if( running ) {
    my_CA( buf );
    displayed = false;
  }
  else displayed = true;
  SDL_Flip(screen);
  if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
}

void run_pause() {
  running = !running;
}

void restart() {
  global_context->buf->reset( *(global_context->img) );
  displayed = false;
}

void slider_value( int value ) {
  //emscripten_run_script("console.log('slider value: ' + value);");
  global_context->my_rule->max_diff = value;
}

int main(int argc, char** argv) {
  ucolor white, black;
  ::white( white ); ::black( black );

  ucolor on = 0x00ffffff;
  ucolor off = 0x00000000;

  // vec2i dims( { 512, 512 } );
  uimage img( "samples/crab_nebula.jpg" );
  auto dims = img.get_dim();
  ucolor* base = img.get_base_ptr();
  // fill image with random black and white pixels (for life test)
  /*for( int i=0; i< dims.x * dims.y; i++ ) {
    if( weighted_bit( 0.33f ) ) base[ i ] = on;
    else base[ i ] = off;
  }*/
/*
  for( int y=0; y <  dims.y; y++ ) {
    for( int x=0; x <  dims.x; x++ ) {
      // white square of diffusable pixels
      if( x > dims.x / 4 && x < dims.x * 3 / 4 && y > dims.y / 4 && y < dims.y * 3 / 4 ) base[ y * dims.x + x ] = on;
      // red cup of non-diffusable pixels
      else if( x >= dims.x * 3 / 4 && x < dims.x * 3 / 4 + 10 && y > dims.y / 4 && y < dims.y * 3 / 4 ) base[ y * dims.x + x ] = 0xff0000ff;
      else if( x <= dims.x / 4 && x > dims.x / 4 - 10 && y > dims.y / 4 && y < dims.y * 3 / 4 ) base[ y * dims.x + x ] = 0xff0000ff;
      else if( x >  dims.x / 4 - 10 && x < dims.x * 3 / 4 + 10 && y >= dims.y * 3 / 4 && y < dims.y * 3 / 4 + 10 ) base[ y * dims.x + x ] = 0xff0000ff;
      // invisible barrier around edge of screen
      else if( x == 0 || x == dims.x - 1 || y == 0 || y == dims.y - 1 ) base[ y * dims.x + x ] = 0xff000000;
      else base[ y * dims.x + x ] = off;
    }
  }
*/

  for( int y=0; y <  dims.y; y++ ) {
    for( int x=0; x <  dims.x; x++ ) {
      // invisible barrier on bottom edge of screen
      if( y == dims.y - 1 ) base[ y * dims.x + x ] = 0xff000000;
      else base[ y * dims.x + x ] = base[ y * dims.x + x ] & 0x00ffffff;
    }
  } 

  //life< ucolor > lifer( on, off );
  std::shared_ptr< rule_pixel_sort< ucolor >  > sorter( new rule_pixel_sort< ucolor >( D4_DOWN, true ) ); // alpha block enabled
 // CA< ucolor > my_CA( sort_wrapper, sorter.neighborhood );
  CA< ucolor > my_CA( std::ref( *sorter ), sorter->neighborhood );
  //pixel_sort< ucolor > pixel_sort( DOWN, true, 300 ); // alpha block enabled
  //CA< ucolor > my_CA( pixel_sort, pixel_sort.neighborhood );
  buffer_pair< ucolor > buf( img );

  SDL_Init(SDL_INIT_VIDEO);
  SDL_Surface *screen = SDL_SetVideoMode(dims.x, dims.y, 32, SDL_SWSURFACE);

  // pack context
  frame_context context;
  context.screen = screen;
  context.my_CA = &my_CA;
  context.my_rule = sorter;
  context.img = &img;
  context.buf = &buf;
  context.dims = &dims;
  global_context = &context;

#ifdef TEST_SDL_LOCK_OPTS
  EM_ASM("SDL.defaults.copyOnLock = false; SDL.defaults.discardOnLock = true; SDL.defaults.opaqueFrontBuffer = false;");
#endif
  emscripten_set_main_loop_arg(iterate_and_display, &context, -1, 1);

  SDL_Quit();

  return 0;
}

EMSCRIPTEN_BINDINGS(my_module) {
    function( "run_pause",    &run_pause );
    function( "restart",      &restart );
    function( "slider_value", &slider_value );
}
