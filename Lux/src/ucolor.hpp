// 32-bit color using unsigned char and bit shifting
// Intended for use with Cellular Automata in Lux Vitae but can be used for other purposes

#ifndef __UCOLOR_HPP
#define __UCOLOR_HPP

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
void apply_mask( ucolor& result, const ucolor& layer, const ucolor& mask );

unsigned long luminance( const ucolor& in );
ucolor gray( const ucolor& in );


#endif // __UCOLOR_HPP