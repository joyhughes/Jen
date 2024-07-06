// Floating point color using vectors from linalg.h
// Uses look up table to convert to and from integer RGB colors

#include "frgb.hpp"
#include "gamma_lut.hpp"

static gamma_LUT glut( 2.2f );

float rf( const frgb &c )       { return c.R; }
float gf( const frgb &c )       { return c.G; }
float bf( const frgb &c )       { return c.B; }

// returns single bytes per component - assumes [0.0, 1.0] range
// clip or constrain out of range values before using
unsigned char rc( const frgb &c ) {  return (unsigned char)glut.linear_to_SRGB( c.R ); }
unsigned char gc( const frgb &c ) {  return (unsigned char)glut.linear_to_SRGB( c.G ); }
unsigned char bc( const frgb &c ) {  return (unsigned char)glut.linear_to_SRGB( c.B ); }

// unsigned int ul() {} // bit shifty stuff

// set component
void setrf( frgb &c, const float& r )   { c.R = r; }
void setgf( frgb &c, const float& g )   { c.G = g; }
void setbf( frgb &c, const float& b )   { c.B = b; }
void setf(  frgb &c, const float& r, const float& g, const float& b) { c.R = r;  c.G = g;  c.B = b; }
frgb setf(  const float& r, const float& g, const float& b ) { return { r, g, b }; }

// TODO - set from bracketed list

void setrc( frgb &c, const unsigned char& r ) { c.R = glut.SRGB_to_linear( r ); }
void setgc( frgb &c, const unsigned char& g ) { c.G = glut.SRGB_to_linear( g ); }
void setbc( frgb &c, const unsigned char& b ) { c.B = glut.SRGB_to_linear( b ); }
void setc(  frgb &c, const unsigned char& r, const unsigned char& g, const unsigned char& b ) 
{ setrc( c, r );   setgc( c, g );   setbc( c, b ); }
frgb fsetc( const unsigned char& r, const unsigned char& g, const unsigned char& b )
{ return { glut.SRGB_to_linear( r ), glut.SRGB_to_linear( g ), glut.SRGB_to_linear( b ) }; }

void setu( frgb &c, const unsigned int& u ) {
    c.R = glut.SRGB_to_linear( (unsigned char)( ( u & 0x00ff0000 ) >> 16 ) );
    c.G = glut.SRGB_to_linear( (unsigned char)( ( u & 0x0000ff00 ) >> 8 ) );
    c.B = glut.SRGB_to_linear( (unsigned char)(   u & 0x000000ff ) );
}

frgb fsetu( const unsigned int& u ) {
    return { glut.SRGB_to_linear( (unsigned char)( ( u & 0x00ff0000 ) >> 16 ) ),
             glut.SRGB_to_linear( (unsigned char)( ( u & 0x0000ff00 ) >> 8 ) ),
             glut.SRGB_to_linear( (unsigned char)(   u & 0x000000ff ) ) };
}

//void setul( unsigned int in ) {} // bit shifty stuff

// Clip to range [ 0.0, 1.0 ] but keep colors in proportion
void constrain( frgb &c ) {
    // find maximum color
    float maxc = linalg::maxelem( c );
    if( maxc > 1.0f ) {
        // bring negative colors to 
        linalg::clamp( c, 0.0f, maxc );
        // if max color > 1.0 divide all colors by max
        c /= maxc;
    }
}

void apply_mask( frgb &result, const frgb &layer, const frgb &mask, const mask_mode& mmode ) {
    switch( mmode ) {
        case MASK_NOEFFECT: result += layer; break;
        case MASK_TRIM:     result += linalg::cmul( layer, mask ); break;
        case MASK_OPACITY:  result =  linalg::cmul( result, ( frgb({ 1.0f, 1.0f, 1.0f }) - mask ) ) + layer; break;
        case MASK_BLEND:    result =  linalg::lerp( result, layer, mask ); break;
    }
    /*
    if( mmode % 2 )  {   // mask affects 
        if( mmode % 1 ) result = linalg::lerp( result, layer, mask );
        else result += linalg::cmul( layer, mask );
    }
    else{
        if( mmode % 1 ) result = linalg::cmul( result, ( frgb({ 1.0f, 1.0f, 1.0f }) - mask ) )  + layer;
        else result += layer;   // mask has no effect
    }   */ 
}

void print_SRGB( const frgb &c ) {
    int r = ( int )rc( c );
    int g = ( int )gc( c );
    int b = ( int )bc( c );
    std::cout << "SRGB   - R: " << r << " G: " << g << " B: " << b;
}

// Returns approximate visual brightness of color
float luminance( const frgb &c ) { return ( 0.299 * c.R + 0.587 * c.G + 0.114 * c.B ); } 

// Grays out the color
frgb gray( const frgb &c ) { 
    auto l = luminance( c );
    return { l, l, l };
}

// Future - can value be made equivalent to luminance? How would this affect the inverse function?
frgb rgb_to_hsv( const frgb& in ) {
    float max = linalg::maxelem( in );
    float min = linalg::minelem( in );
    float h, s, v;
    v = max; // Value is maximum of RGB components.

    float delta = max - min;
    if (delta == 0.0f) {
        s = 0;
        h = 0; // h is undefined.
        return frgb( { h, s, v } );
    }

    if (max != 0.0) { // NOTE: if Max is == 0, this divide would cause a crash
        s = (delta / max); // s
    } else {
        // If max is 0, then r = g = b = 0              
        s = 0.0f;
        h = 0.0f; // its now undefined
        return frgb( { h, s, v } );
    }

    if ( rf( in ) >= max ) {
        h = ( gf( in ) - bf( in ) ) / delta; // between yellow & magenta
    } else if ( gf( in ) >= max) {
        h = 2.0f + ( bf( in ) - rf( in ) ) / delta; // between cyan & yellow
    } else {
        h = 4.0f + ( rf( in ) - gf( in ) ) / delta; // between magenta & cyan
    }

    h *= 60.0f; // degrees

    if (h < 0.0f) {
        h += 360.0f;
    }

    return frgb( { h, s, v } );
}

frgb hsv_to_rgb( const frgb& in ) {
    float s = gf( in );
    float v = bf( in );

    if( s <= 0.0f ) return frgb( { v, v, v } );  // < is bogus, just shuts up warnings

    float hh = rf( in );
    if( hh >= 360.0f ) hh = 0.0f;
    hh /= 60.0f;
    float i = floorf( hh );
    float ff = hh - i;

    float p = v * ( 1.0f - s );
    float q = v * ( 1.0f - (s * ff) );
    float t = v * ( 1.0f - (s * (1.0f - ff) ));

    if( i == 0.0f ) return frgb( { v, t, p } );
    if( i == 1.0f ) return frgb( { q, v, p } );
    if( i == 2.0f ) return frgb( { p, v, t } );
    if( i == 3.0f ) return frgb( { p, q, v } );
    if( i == 4.0f ) return frgb( { t, p, v } );
    else            return frgb( { v, p, q } );
}