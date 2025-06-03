// 32-bit color using unsigned char and bit shifting
// Intended for use with Cellular Automata in Lux Vitae but can be used for other purposes

#ifndef __UCOLOR_HPP
#define __UCOLOR_HPP

#include <vector>
#include <algorithm>
#include "mask_mode.hpp"

typedef unsigned int ucolor;

float af( const ucolor &c );
float rf( const ucolor &c );
float gf( const ucolor &c );
float bf( const ucolor &c );

// returns single bytes per component
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


void apply_mask( ucolor& result, const ucolor& layer, const ucolor& mask, const mask_mode& mmode = MASK_BLEND  );
ucolor blend( const ucolor& a, const ucolor& b );
ucolor blend( const ucolor& a, const ucolor& b, const ucolor& c, const ucolor& d );
ucolor blend( const std::vector< ucolor >& colors );

// proportion 0-256 -> 0-100%
static inline ucolor blend( const ucolor& a, const ucolor& b, const unsigned int& prop )
{
    // should a random bit be included and at what probability?
    // unsigned int random_bit = fair_coin( gen );
    unsigned int iprop = 256-prop;

    return 
        ( a & 0xff000000 ) |
    ( ( ( a & 0x00ff0000 ) * prop + ( b & 0x00ff0000 ) * iprop ) >> 8 & 0x00ff0000 ) |
    ( ( ( a & 0x0000ff00 ) * prop + ( b & 0x0000ff00 ) * iprop ) >> 8 & 0x0000ff00 ) |
    ( ( ( a & 0x000000ff ) * prop + ( b & 0x000000ff ) * iprop ) >> 8 );
}


static inline ucolor blendf(  const ucolor& a, const ucolor& b, const float& prop ) 
{ return blend( a, b, (unsigned int)( prop * 256.0f ) ); }


inline unsigned long luminance( const ucolor& in ) {
    return( ( (   shift_right_2( in ) + shift_right_4( in ) ) +            // r * 5/16
              ( ( shift_right_1( in ) + shift_right_4( in ) ) >> 8  ) +    // g * 9/16
                (  shift_right_3( in ) >> 16 ) )                           // b * 2/16
                & 0x000000ff );                   
}

ucolor gray( const ucolor& in );

//inline void white( ucolor& w ) { w = 0xffffffff; }
//inline void black( ucolor& b ) { b = 0xff000000; } // Alpha channel set

static inline unsigned int manhattan( const ucolor& a, const ucolor& b )
{
   unsigned int out = 0;

   if( ( a & 0x00ff0000 ) > ( b & 0x00ff0000 ) ) out += ( ( a & 0x00ff0000 ) >> 16 ) - ( ( b & 0x00ff0000 ) >> 16 );
      else out += ( ( b & 0x00ff0000 ) >> 16 ) - ( ( a & 0x00ff0000 ) >> 16 );

   if( ( a & 0x0000ff00 ) > ( b & 0x0000ff00 ) ) out += ( ( a & 0x0000ff00 ) >> 8 ) - ( ( b & 0x0000ff00 ) >> 8 );
      else out +=  ( ( b & 0x0000ff00 ) >> 8 ) - ( ( a & 0x0000ff00 ) >> 8 );

   if( ( a & 0x000000ff ) > ( b & 0x000000ff ) ) out += ( a & 0x000000ff ) - ( b & 0x000000ff );
      else out += ( b & 0x000000ff ) - ( a & 0x000000ff );

   return out;
}

// add colors, handling overflow
static inline void addc( ucolor& c1, const ucolor& c2 ) {
   c1 = ( c1 & 0xff000000 ) + 
      std::min< unsigned int>( ( c1 & 0x00ff0000 ) + ( c2 & 0x00ff0000 ), 0x00ff0000 ) +
      std::min< unsigned int>( ( c1 & 0x0000ff00 ) + ( c2 & 0x0000ff00 ), 0x0000ff00 ) +
      std::min< unsigned int>( ( c1 & 0x000000ff ) + ( c2 & 0x000000ff ), 0x000000ff );
}

// subtract colors, handling underflow
static inline void subc( ucolor& c1, const ucolor& c2 )
{
   unsigned int r, g, b;
   if( ( c1 & 0x00ff0000 ) > ( c2 & 0x00ff0000 ) ) r = ( c1 & 0x00ff0000 ) - ( c2 & 0x00ff0000 );
      else r = 0;
   if( ( c1 & 0x0000ff00 ) > ( c2 & 0x0000ff00 ) ) g = ( c1 & 0x0000ff00 ) - ( c2 & 0x0000ff00 );
      else g = 0;
   if( ( c1 & 0x000000ff ) > ( c2 & 0x000000ff ) ) b = ( c1 & 0x000000ff ) - ( c2 & 0x000000ff );
      else b = 0;

   // preserve alpha channel
   c1 = ( c1 & 0xff000000 ) + r + g + b;
}

// multiply colors, handling overflow
static inline ucolor mulc( const ucolor& c1, const ucolor& c2 )
{
   return ( c1 & 0xff000000 ) + 
      std::min< unsigned int>( ( ( c1 & 0x00ff0000 ) * ( c2 & 0x00ff0000 ) ) >> 8, 0x00ff0000 ) +
      std::min< unsigned int>( ( ( c1 & 0x0000ff00 ) * ( c2 & 0x0000ff00 ) ) >> 8, 0x0000ff00 ) +
      std::min< unsigned int>( ( ( c1 & 0x000000ff ) * ( c2 & 0x000000ff ) ) >> 8, 0x000000ff );
}

static inline void rotate_components( ucolor& c, const int& r )
{
   if(      !r%1 ) c = ( c & 0xff000000 ) | ( c & 0x00ffff00 >> 8 ) | ( c & 0x000000ff << 16 );
   else if( !r%2 ) c = ( c & 0xff000000 ) | ( c & 0x0000ffff << 8 ) | ( c & 0x00ff0000 >> 16 );
 
}

static inline ucolor rotate_components( const ucolor& c, const int& r )
{
   if(      !r%1 ) return ( c & 0xff000000 ) | ( c & 0x00ffff00 >> 8 ) | ( c & 0x000000ff << 16 );
   else if( !r%2 ) return ( c & 0xff000000 ) | ( c & 0x0000ffff << 8 ) | ( c & 0x00ff0000 >> 16 );
   else return c;
}

static inline void invert( ucolor& c )
{
   c = ( c & 0xff000000 ) | ( ( ~c ) & 0x00ffffff );
}

static inline ucolor invert( const ucolor& c )
{
   return ( c & 0xff000000 ) | ( ( ~c ) & 0x00ffffff );
}

ucolor rgb_to_hsv( const ucolor& c );
ucolor hsv_to_rgb( const ucolor& c );

// Already shifted to second most significant byte
static inline ucolor rotate_hue( const ucolor& c, const unsigned int& r )
{
   return ( ( ( c & 0x00ff0000 ) + r ) & 0x00ff0000 ) | ( c & 0xff00ffff );
}

static inline ucolor bit_plane( const ucolor& c, const ucolor& q)
{
   ucolor m = c & q;
   //std::cout << "c: " << std::hex << c << " m: " << std::hex << m << std::endl;
   ucolor out = c & 0xff000000;
   unsigned int count =  ((m & 0x00800000) >> 23 ) + ((m & 0x00400000) >> 22 ) + ((m & 0x00200000) >> 21) + ((m & 0x00100000) >> 20) +
                         ((m & 0x00080000) >> 19 ) + ((m & 0x00040000) >> 18 ) + ((m & 0x00020000) >> 17) + ((m & 0x00010000) >> 16);
   unsigned int qcount = ((q & 0x00800000) >> 23 ) + ((q & 0x00400000) >> 22 ) + ((q & 0x00200000) >> 21) + ((q & 0x00100000) >> 20) +
                         ((q & 0x00080000) >> 19 ) + ((q & 0x00040000) >> 18 ) + ((q & 0x00020000) >> 17) + ((q & 0x00010000) >> 16);
   if( qcount ) out += ( 0x00ff0000 * count / qcount ) & 0x00ff0000;
   //std::cout << "red: " << std::hex << out << " " << " count " << count << " qcount " << qcount << std::endl;
   count =  ((m & 0x00008000) >> 15 ) + ((m & 0x00004000) >> 14 ) + ((m & 0x00002000) >> 13) + ((m & 0x00001000) >> 12) +
            ((m & 0x00000800) >> 11 ) + ((m & 0x00000400) >> 10 ) + ((m & 0x00000200) >>  9) + ((m & 0x00000100) >>  8);
   qcount = ((q & 0x00008000) >> 15 ) + ((q & 0x00004000) >> 14 ) + ((q & 0x00002000) >> 13) + ((q & 0x00001000) >> 12) +
            ((q & 0x00000800) >> 11 ) + ((q & 0x00000400) >> 10 ) + ((q & 0x00000200) >>  9) + ((q & 0x00000100) >>  8);
   if( qcount ) out += ( 0x0000ff00 * count / qcount ) & 0x0000ff00;
   //std::cout << "green: " << std::hex << out << " " << " count " << count << " qcount " << qcount << std::endl;
   count =  ((m & 0x00000080) >> 7 ) + ((m & 0x00000040) >> 6 ) + ((m & 0x00000020) >> 5 ) + ((m & 0x00000010) >> 4 ) +
            ((m & 0x00000008) >> 3 ) + ((m & 0x00000004) >> 2 ) + ((m & 0x00000002) >> 1 ) +  (m & 0x00000001);
   qcount = ((q & 0x00000080) >> 7 ) + ((q & 0x00000040) >> 6 ) + ((q & 0x00000020) >> 5 ) + ((q & 0x00000010) >> 4 ) +
            ((q & 0x00000008) >> 3 ) + ((q & 0x00000004) >> 2 ) + ((q & 0x00000002) >> 1 ) +  (q & 0x00000001);
   if( qcount ) out += ( 0x000000ff * count / qcount ) & 0x000000ff;
   //std::cout << "blue: " << std::hex << out << " " << " count " << count << " qcount " << qcount << std::endl;
   //std::cout << "out: " << std::hex << ( c & 0xff000000 ) + out << std::endl;
   return out;
}

#endif // __UCOLOR_HPP