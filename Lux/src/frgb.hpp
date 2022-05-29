#ifndef __FRGB_HPP
#define __FRGB_HPP

#include <iostream>
//#include <array>
//#include <algorithm>
#include "linalg.h"

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

float r( const frgb &c );
float g( const frgb &c );
float b( const frgb &c );

// returns single bytes per component - assumes [0.0, 1.0] range
// clip or constrain out of range values before using
unsigned char rc( const frgb &c );
unsigned char gc( const frgb &c );
unsigned char bc( const frgb &c );

// inline unsigned int ul( const frgb &c ) {} // bit shifty stuff

// set component
void setr( frgb &c, const float& r );
void setg( frgb &c, const float& g );
void setb( frgb &c, const float& b );
void set(  frgb &c, const float& r, const float& g, const float& b );
frgb set(  const float& r, const float& g, const float& b );
// TODO - set from bracketed list

void setrc( frgb &c, const unsigned char& r );
void setgc( frgb &c, const unsigned char& g );
void setbc( frgb &c, const unsigned char& b );
void setc(  frgb &c, const unsigned char& r, const unsigned char& g, const unsigned char& b );
frgb setc(  const unsigned char& r, const unsigned char& g, const unsigned char& b );

// I/O operators
//std::ostream &operator << ( std::ostream &out, const frgb& f );
void print_SRGB( const frgb &c );

// to clamp values use linalg::clamp()
// component-wise clamp?
void constrain( frgb &c );  // Clip to range [ 0.0, 1.0 ] but keep colors in proportion
// frgb constrain( const frgb &c );   

void apply_mask( const frgb &base, const frgb &mask, frgb &result );
// frgb apply_mask( const frgb &base, const frgb &mask, frgb &result );

inline float luminance( const frgb &c );  // Returns approximate visual brightness of color
frgb gray( const frgb &c );        // Grays out the color

// Future: HSV and other color spaces

#endif // __FRGB_HPP