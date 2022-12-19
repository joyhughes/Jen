#include <SDL.h>
#include "life.hpp"
#include "uimage.hpp"
#include <emscripten.h>

// used for stuffing everything needed to iterate and display a frame into a single void*
struct frame_context {
  SDL_Surface *screen;
  life< ucolor > *lifer;
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
  life< ucolor >& lifer = *(context->lifer);
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
  lifer( buf );
  SDL_Flip(screen);
  if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
}

int main(int argc, char** argv) {
  ucolor white, black;
  ::white( white ); ::black( black );

  vec2i dims( { 512, 512 } );
  uimage img( dims );
  ucolor* base = img.get_base();
  // fill image with random black and white pixels
  for( int i=0; i< dims.x * dims.y; i++ ) {
    if( weighted_bit( 0.33f ) ) base[ i ] = 0xffffffff;
    else base[ i ] = 0xff000000;
  }

  life< ucolor > lifer;
  buffer_pair< ucolor > buf( img );

  SDL_Init(SDL_INIT_VIDEO);
  SDL_Surface *screen = SDL_SetVideoMode(dims.x, dims.y, 32, SDL_SWSURFACE);

  // pack context
  frame_context context;
  context.screen = screen;
  context.lifer = &lifer;
  context.buf = &buf;
  context.dims = &dims;

#ifdef TEST_SDL_LOCK_OPTS
  EM_ASM("SDL.defaults.copyOnLock = false; SDL.defaults.discardOnLock = true; SDL.defaults.opaqueFrontBuffer = false;");
#endif
  emscripten_set_main_loop_arg(iterate_and_display, &context, -1, 1);

  SDL_Quit();

  return 0;
}