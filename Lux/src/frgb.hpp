// Floating point color using vectors from linalg.h
// Uses look up table to convert to and from integer RGB colors

#ifndef __FRGB_HPP
#define __FRGB_HPP

#include <iostream>
#include "linalg.h"
#include "mask_mode.hpp"

#ifndef R
    #define R x 
#endif // R
#ifndef G
    #define G y
#endif // G
#ifndef B
    #define B z
#endif // B

typedef linalg::vec< float,3 > frgb;

float rf( const frgb &c );
float gf( const frgb &c );
float bf( const frgb &c );

// returns single bytes per component - assumes [0.0, 1.0] range
// clip or constrain out of range values before using
unsigned char rc( const frgb &c );
unsigned char gc( const frgb &c );
unsigned char bc( const frgb &c );

// inline unsigned int ul( const frgb &c ) {} // bit shifty stuff

// set component
void setrf( frgb &c, const float& r );
void setgf( frgb &c, const float& g );
void setbf( frgb &c, const float& b );
void setf(  frgb &c, const float& r, const float& g, const float& b );
frgb setf(  const float& r, const float& g, const float& b );
// TODO - set from bracketed list

void setrc( frgb &c, const unsigned char& r );
void setgc( frgb &c, const unsigned char& g );
void setbc( frgb &c, const unsigned char& b );
void setc(  frgb &c, const unsigned char& r, const unsigned char& g, const unsigned char& b );
frgb fsetc(          const unsigned char& r, const unsigned char& g, const unsigned char& b );

// convert ucolor to frgb
void setu( frgb &c, const unsigned int &uc );
frgb fsetu( const unsigned int &c );

// I/O operators
//std::ostream &operator << ( std::ostream &out, const frgb& f );
void print_SRGB( const frgb &c );

// to clamp values use linalg::clamp()
void constrain( frgb &c );  // Clip to range [ 0.0, 1.0 ] but keep colors in proportion
//frgb& constrain( const frgb &c );   

void apply_mask( frgb& result, const frgb& layer, const frgb& mask, const mask_mode& mmode = MASK_BLEND );
// frgb apply_mask( const frgb &base, const frgb &mask, frgb &result );

inline float luminance( const frgb &c );  // Returns approximate visual brightness of color
frgb gray( const frgb &c );        // Grays out the color
//inline void white( frgb& w ) { w = frgb( { 1.0f, 1.0f, 1.0f  } ); }
//inline void black( frgb& b ) { b = frgb( { 0.0f, 0.0f, 0.0f  } ); }
static inline frgb blendf( const frgb& a, const frgb& b, const float& prop ) { return linalg::lerp( a, b, prop ); }
static inline frgb blend( const frgb& a, const frgb& b ) { return ( a + b ) / 2; }

// Wrappers for addition and subtraction - used for image compatibility
static inline void addc( frgb& c1, const frgb& c2 ) { c1 += c2; }
static inline void subc( frgb& c1, const frgb& c2 ) { c1 -= c2; }
static inline frgb mulc( const frgb& c1, const frgb& c2 ) { return linalg::cmul( c1, c2 ); }

static inline void rotate_components( frgb& c, const int& r )
{
    if(      !r%1 ) c = frgb( { c.R, c.G, c.B } );
    else if( !r%2 ) c = frgb( { c.B, c.R, c.G } );
}

static inline frgb rotate_components( const frgb& c, const int& r )
{
    if(      !r%1 ) return frgb( { c.R, c.G, c.B } );
    else if( !r%2 ) return frgb( { c.B, c.R, c.G } );
    else return c;
}

static inline void invert( frgb& c )
{
    c = frgb( { 1.0f - c.R, 1.0f - c.G, 1.0f - c.B } );
}

static inline frgb invert( const frgb& c )
{
    return frgb( { 1.0f - c.R, 1.0f - c.G, 1.0f - c.B } );
}

frgb rgb_to_hsv( const frgb& in );
frgb hsv_to_rgb( const frgb& in );

static inline frgb rotate_hue( const frgb& c, const float& h )
{
    return frgb( tmodf( c.R + h, 360.0 ), c.G, c.B );
}

#endif // __FRGB_HPP
