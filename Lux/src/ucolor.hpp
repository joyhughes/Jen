// 32-bit color using unsigned char and bit shifting
// Intended for use with Cellular Automata in Lux Vitae but can be used for other purposes

#ifndef __UCOLOR_HPP
#define __UCOLOR_HPP

#include "mask_mode.hpp"
typedef unsigned int ucolor;

float af( const ucolor &c );
float rf( const ucolor &c );
float gf( const ucolor &c );
float bf( const ucolor &c );

// returns single bytes per component - assumes [0.0, 1.0] range
// clip or constrain out of range values before using
unsigned char ac( const ucolor &c );    // alpha channel
unsigned char rc( const ucolor &c );
unsigned char gc( const ucolor &c );
unsigned char bc( const ucolor &c );

// set component
void setaf( ucolor &c, const float& b );
void setrf( ucolor &c, const float& r );
void setgf( ucolor &c, const float& g );
void setbf( ucolor &c, const float& b );
void setf(  ucolor &c, const float& r, const float& g, const float& b );
ucolor usetf(          const float& r, const float& g, const float& b );
// TODO - set from bracketed list

void setac( ucolor &c, const unsigned char& b );
void setrc( ucolor &c, const unsigned char& r );
void setgc( ucolor &c, const unsigned char& g );
void setbc( ucolor &c, const unsigned char& b );
void setc(  ucolor &c, const unsigned char& r, const unsigned char& g, const unsigned char& b );
void setc(  ucolor &c, const unsigned char& a, const unsigned char& r, const unsigned char& g, const unsigned char& b );
ucolor usetc(          const unsigned char& r, const unsigned char& g, const unsigned char& b );

ucolor shift_right_1( const ucolor &c );
ucolor shift_right_2( const ucolor &c );
ucolor shift_right_3( const ucolor &c );
ucolor shift_right_4( const ucolor &c );
ucolor shift_right_5( const ucolor &c );
ucolor shift_right_6( const ucolor &c );
ucolor shift_right_7( const ucolor &c );

ucolor blend( const ucolor& a, const ucolor& b );
void apply_mask( ucolor& result, const ucolor& layer, const ucolor& mask, const mask_mode& mmode = MASK_BLEND  );

unsigned long luminance( const ucolor& in );
ucolor gray( const ucolor& in );

inline void white( ucolor& w ) { w = 0xffffffff; }
inline void black( ucolor& b ) { b = 0xff000000; } // Alpha channel set

inline ucolor manhattan( const ucolor& a, const ucolor& b )
{
   unsigned int out = 0;

   if( ( a & 0x00ff0000 ) > ( b & 0x00ff0000 ) ) out += ( ( a & 0x00ff0000 ) >> 16 ) - ( ( b & 0x00ff0000 ) >> 16 );
      else out += ( ( b & 0x00ff0000 ) >> 16 ) - ( ( a & 0x00ff0000 ) >> 16 );

   if( ( a & 0x0000ff00 ) > ( b & 0x0000ff00 ) ) out += ( ( a & 0x0000ff00 ) >> 8 ) - ( ( b & 0x0000ff00 ) >> 8 );
      else out +=  ( ( b & 0x0000ff00 ) >> 8 ) - ( ( a & 0x0000ff00 ) >> 8 );

   if( ( a & 0x000000ff ) > ( b & 0x000000ff ) ) out += ( a & 0x000000ff ) - ( b & 0x000000ff );
      else out += ( b & 0x0000ff00 ) - ( a & 0x0000ff00 );

   return out;
}

/*inline ucolor blend( const ucolor& a, const ucolor& b )
{
   unsigned int random_bit = rand() % 2;

   return
   ( ( ( ( a & 0x00ff0000 ) + ( b & 0x00ff0000 ) + 0x00010000 * random_bit ) >> 1 ) & 0x00ff0000 ) + 
   ( ( ( ( a & 0x0000ff00 ) + ( b & 0x0000ff00 ) + 0x00000100 * random_bit ) >> 1 ) & 0x0000ff00 ) +
   ( ( ( ( a & 0x000000ff ) + ( b & 0x000000ff ) + 0x00000001 * random_bit ) >> 1 ) & 0x000000ff ) +
   0xff000000;
}*/

#endif // __UCOLOR_HPP