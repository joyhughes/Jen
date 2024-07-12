// Color using unsigned char and bit shifting
// Intended for use with Cellular Automata in Lux Vitae but can be used for other purposes
#include <iostream>
#include "ucolor.hpp"
#include "gamma_lut.hpp"
#include "joy_rand.hpp"

static gamma_LUT glut( 2.2f );

float af( const ucolor &c )       { return ac( c ) / 255.0f; }
float rf( const ucolor &c )       { return glut.SRGB_to_linear( rc( c ) ); }
float gf( const ucolor &c )       { return glut.SRGB_to_linear( gc( c ) ); }
float bf( const ucolor &c )       { return glut.SRGB_to_linear( bc( c ) ); }

// returns single bytes per component 
unsigned char ac( const ucolor &c ) {  return (unsigned char) ( ( c >> 24 ) & 0xff ); }
unsigned char rc( const ucolor &c ) {  return (unsigned char) ( ( c >> 16 ) & 0xff ); }
unsigned char gc( const ucolor &c ) {  return (unsigned char) ( ( c >>  8 ) & 0xff ); }
unsigned char bc( const ucolor &c ) {  return (unsigned char) ( ( c       ) & 0xff ); }

// set component
void setaf( ucolor &c, const float& a )   { setac( c, ( unsigned char )( std::clamp( a, 0.0f, 1.0f ) * 255.0f ) ); }
void setrf( ucolor &c, const float& r )   { setrc( c, glut.linear_to_SRGB( r ) ); }
void setgf( ucolor &c, const float& g )   { setgc( c, glut.linear_to_SRGB( g ) ); }
void setbf( ucolor &c, const float& b )   { setbc( c, glut.linear_to_SRGB( b ) ); }
void setf(  ucolor &c, const float& r, const float& g, const float& b) { setc( c, glut.linear_to_SRGB( r ), glut.linear_to_SRGB( g ), glut.linear_to_SRGB( b )); }
ucolor usetf( const float& r, const float& g, const float& b ) { return usetc(    glut.linear_to_SRGB( r ), glut.linear_to_SRGB( g ), glut.linear_to_SRGB( b )); }

void setac( ucolor &c, const unsigned char& a ) { c = ( c & 0x00ffffff ) | ( ( (unsigned int)a ) << 24 ); }
void setrc( ucolor &c, const unsigned char& b ) { c = ( c & 0xffffff00 ) | (   (unsigned int)b         ); }
void setgc( ucolor &c, const unsigned char& g ) { c = ( c & 0xffff00ff ) | ( ( (unsigned int)g ) << 8  ); }
void setbc( ucolor &c, const unsigned char& r ) { c = ( c & 0xff00ffff ) | ( ( (unsigned int)r ) << 16 ); }

void setc(  ucolor &c, const unsigned char& r, const unsigned char& g, const unsigned char& b ) 
{ c = c & 0xff000000 | ( (unsigned int)r << 16 ) | ( (unsigned int)g << 8  ) | ( (unsigned int)b       ); }

void setc(  ucolor &c, const unsigned char& a, const unsigned char& r, const unsigned char& g, const unsigned char& b ) 
{ c = ( (unsigned int)a << 24 ) | ( (unsigned int)r << 16 ) | ( (unsigned int)g << 8  ) | ( (unsigned int)b       ); }

ucolor usetc( const unsigned char& r, const unsigned char& g, const unsigned char& b )
{ return ( 0xff000000 | (unsigned int)r << 16 ) | ( (unsigned int)g << 8  ) | ( (unsigned int)b       ); }

/*
void shift_right_1( ucolor &c ) { c = ( c >> 1 ) & 0x7f7f7f7f; }
void shift_right_2( ucolor &c ) { c = ( c >> 1 ) & 0x3f3f3f3f; }
void shift_right_3( ucolor &c ) { c = ( c >> 1 ) & 0x1f1f1f1f; }
void shift_right_4( ucolor &c ) { c = ( c >> 1 ) & 0x0f0f0f0f; }
void shift_right_5( ucolor &c ) { c = ( c >> 1 ) & 0x07070707; }
void shift_right_6( ucolor &c ) { c = ( c >> 1 ) & 0x03030303; }
void shift_right_7( ucolor &c ) { c = ( c >> 1 ) & 0x01010101; }
*/

ucolor shift_right_1( const ucolor &c ) { return ( c >> 1 ) & 0x7f7f7f7f; }
ucolor shift_right_2( const ucolor &c ) { return ( c >> 2 ) & 0x3f3f3f3f; }
ucolor shift_right_3( const ucolor &c ) { return ( c >> 3 ) & 0x1f1f1f1f; }
ucolor shift_right_4( const ucolor &c ) { return ( c >> 4 ) & 0x0f0f0f0f; }
ucolor shift_right_5( const ucolor &c ) { return ( c >> 5 ) & 0x07070707; }
ucolor shift_right_6( const ucolor &c ) { return ( c >> 6 ) & 0x03030303; }
ucolor shift_right_7( const ucolor &c ) { return ( c >> 7 ) & 0x01010101; }

// todo - implement different mask modes 
// currently only handles MASK_BLEND
void apply_mask( ucolor& result, const ucolor& layer, const ucolor& mask, const mask_mode& mmode ) {
    result =     ( result & 0xff000000 ) |
             ( ( ( ( result & 0x00ff0000 ) * ( 0xff - ( ( mask & 0x00ff0000 ) >> 16 ) ) + ( layer & 0x00ff0000 ) * ( ( mask & 0x00ff0000 ) >> 16 ) ) >> 8 ) & 0x00ff0000 ) |
             ( ( ( ( result & 0x0000ff00 ) * ( 0xff - ( ( mask & 0x0000ff00 ) >>  8 ) ) + ( layer & 0x0000ff00 ) * ( ( mask & 0x0000ff00 ) >>  8 ) ) >> 8 ) & 0x0000ff00 ) |
             ( ( ( ( result & 0x000000ff ) * ( 0xff - ( ( mask & 0x000000ff )       ) ) + ( layer & 0x000000ff ) * ( ( mask & 0x000000ff )       ) ) >> 8 ) & 0x000000ff );
}

ucolor blend( const ucolor& a, const ucolor& b ) {
   // Random bit assures average color stays the same
   unsigned int random_bit = fair_coin( gen );

   return
   ( ( ( ( a & 0x00ff0000 ) + ( b & 0x00ff0000 ) + 0x00010000 * random_bit ) >> 1 ) & 0x00ff0000 ) + 
   ( ( ( ( a & 0x0000ff00 ) + ( b & 0x0000ff00 ) + 0x00000100 * random_bit ) >> 1 ) & 0x0000ff00 ) +
   ( ( ( ( a & 0x000000ff ) + ( b & 0x000000ff ) + 0x00000001 * random_bit ) >> 1 ) & 0x000000ff ) +
   0xff000000;
}

ucolor blend( const ucolor& a, const ucolor& b, const ucolor& c, const ucolor& d ) {
   // Random bit assures average color stays the same

   return
   ( ( ( ( a & 0x00ff0000 ) + ( b & 0x00ff0000 ) + ( c & 0x00ff0000 ) + ( d & 0x00ff0000 ) + 0x00010000 ) >> 2 ) & 0x00ff0000 ) + 
   ( ( ( ( a & 0x0000ff00 ) + ( b & 0x0000ff00 ) + ( c & 0x0000ff00 ) + ( d & 0x0000ff00 ) + 0x00000100 ) >> 2 ) & 0x0000ff00 ) +
   ( ( ( ( a & 0x000000ff ) + ( b & 0x000000ff ) + ( c & 0x000000ff ) + ( d & 0x000000ff ) + 0x00000001 ) >> 2 ) & 0x000000ff ) +
   0xff000000; // blend alphas?
}

// Can blend up to 256 colors
ucolor blend( const std::vector< ucolor >& colors ) {
    unsigned int random_add = rand_uint( gen ) % colors.size();

    unsigned int r = random_add, g = random_add, b = random_add;
    for( unsigned int i = 0; i < colors.size(); i++ ) {
        r += rc( colors[ i ] );
        g += gc( colors[ i ] );
        b += bc( colors[ i ] );
    }
    r /= colors.size();
    g /= colors.size();
    b /= colors.size();

    return ( 0xff000000 | ( r << 16 ) | ( g << 8 ) | b );
}

ucolor gray( const ucolor& in ) {
    unsigned int lum = luminance( in );
    return( ( in & 0xff000000 ) + ( lum << 16 ) + ( lum << 8 ) + lum );
}

ucolor rgb_to_hsv( const ucolor& in ) {
    unsigned int r = ( in >> 16 ) & 0xff;
    unsigned int g = ( in >> 8 ) & 0xff;
    unsigned int b = in & 0xff;
    //std::cout << "rgb_to_hsv: RGB = " << r << " " << g << " " << b << std::endl;
    unsigned int h, s, v;

    unsigned int max = r > g ? r : g;
    max = max > b ? max : b;
    unsigned int min = r < g ? r : g;
    min = min < b ? min : b;

    v = max;
    unsigned int delta = max - min;
    if( delta == 0 ) {
        return ( in & 0xff000000 ) + r; // gray, white, black - implied if delta == 0, also max == 0 and divide by zero will not occur below
    }
    s = delta * 0xff / max;

    // fixed point math for a more accurate result
    if( r == max ) {
        if( b == min )
            h = ( ( ( g - b ) * 0x2AAAAA / delta + 0x7FFF ) >> 16 ) & 0xff;
        else
            h = ( ( 0x1000000 - ( b - g ) * 0x2AAAAA / delta + 0x7FFF ) >> 16 ) & 0xff;
    }
    else if( g == max ) {
        if( r == min )
            h = ( 0x555555 + ( b - r ) * 0x2AAAAA / delta + 0x7FFF ) >> 16;
        else
            h = ( 0x555555 - ( r - b ) * 0x2AAAAA / delta + 0x7FFF ) >> 16;
    }
    else {
        if( g == min )
            h = ( 0xAAAAAB + ( r - g ) * 0x2AAAAA / delta + 0x7FFF ) >> 16;
        else
            h = ( 0xAAAAAB - ( g - r ) * 0x2AAAAA / delta + 0x7FFF ) >> 16;
    }
    //std::cout << "rgb_to_hsv: HSV = " << h << " " << s << " " << v << std::endl;
    return ( ( in & 0xff000000 ) + ( h << 16 ) + ( s << 8 ) + v );
}

ucolor hsv_to_rgb( const ucolor& in ) {
    unsigned int h = ( in >> 16 ) & 0xff;
    unsigned int s = ( in >> 8 ) & 0xff;
    unsigned int v = in & 0xff;

    if( s == 0 ) {
        return ( ( in & 0xff000000 ) + ( v << 16 ) + ( v << 8 ) + v ); // gray
    }

    // fixed point math for a more accurate result
    unsigned int i = ( h * 0x10000 ) / 0x2AAAAB;
    unsigned int f = ( h * 0x100 - i * 0x2AAB ) * 6;
    unsigned int p = ( v * ( 0xff - s) + 0x7f ) >> 8;
    unsigned int q = ( v * ( 0xffff - ( s * f ) / 0x100 ) + 0x7fff ) >> 16;
    unsigned int t = ( v * ( 0xffff - ( s * ( 0xffff - f ) ) / 0x100 ) + 0x7fff ) >> 16;

    switch( i ) {
        case 0: return  ( ( in & 0xff000000 ) + ( v << 16 ) + ( t << 8 ) + p );
        case 1: return  ( ( in & 0xff000000 ) + ( q << 16 ) + ( v << 8 ) + p );
        case 2: return  ( ( in & 0xff000000 ) + ( p << 16 ) + ( v << 8 ) + t );
        case 3: return  ( ( in & 0xff000000 ) + ( p << 16 ) + ( q << 8 ) + v );
        case 4: return  ( ( in & 0xff000000 ) + ( t << 16 ) + ( p << 8 ) + v );
        default: return ( ( in & 0xff000000 ) + ( v << 16 ) + ( p << 8 ) + q );
    }
}
