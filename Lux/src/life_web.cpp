#include <SDL.h>
#include "life.hpp"
#include "uimage.hpp"
#include <emscripten.h>

// used for stuffing everything needed to iterate and display a frame into a single void*
struct frame_context {
  SDL_Surface *screen;
  CA< ucolor > *my_CA;
  buffer_pair< ucolor > *buf;
  vec2i *dims;
};

// used as emscripten main loop
void iterate_and_display(void *arg)
{
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
  unsigned int* base_ptr = buf.get_image().get_base();
  for( int i=0; i< dims.x * dims.y; i++ ) {
    *pixel_ptr = *base_ptr;
    pixel_ptr++; base_ptr++;
  }
  my_CA( buf );
  SDL_Flip(screen);
  if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
}

int main(int argc, char** argv) {
  ucolor white, black;
  ::white( white ); ::black( black );

  ucolor on = white;
  ucolor off = black;

  vec2i dims( { 512, 512 } );
  uimage img( dims );
  ucolor* base = img.get_base();
  // fill image with random black and white pixels
  /*for( int i=0; i< dims.x * dims.y; i++ ) {
    if( weighted_bit( 0.33f ) ) base[ i ] = on;
    else base[ i ] = off;
  }*/

  for( int y=0; y <  dims.y; y++ ) {
    for( int x=0; x <  dims.x; x++ ) {
      if( x > dims.x / 4 && x < dims.x * 3 / 4 && y > dims.y / 4 && y < dims.y * 3 / 4 ) base[ y * dims.x + x ] = on;
      else base[ y * dims.x + x ] = off;
    }
  }

  //life< ucolor > lifer( on, off );
  diffuse< ucolor > diffuser;
  CA< ucolor > my_CA( diffuser, diffuser.neighborhood );
  buffer_pair< ucolor > buf( img );

  SDL_Init(SDL_INIT_VIDEO);
  SDL_Surface *screen = SDL_SetVideoMode(dims.x, dims.y, 32, SDL_SWSURFACE);

  // pack context
  frame_context context;
  context.screen = screen;
  context.my_CA = &my_CA;
  context.buf = &buf;
  context.dims = &dims;

#ifdef TEST_SDL_LOCK_OPTS
  EM_ASM("SDL.defaults.copyOnLock = false; SDL.defaults.discardOnLock = true; SDL.defaults.opaqueFrontBuffer = false;");
#endif
  emscripten_set_main_loop_arg(iterate_and_display, &context, -1, 1);

  SDL_Quit();

  return 0;
}