#include "frgb.hpp"
#include "gamma_lut.hpp"

//using namespace linalg;

static gamma_LUT glut( 2.2f );

inline float r( const frgb &c )       { return c.R; }
inline float g( const frgb &c )       { return c.G; }
inline float b( const frgb &c )       { return c.B; }

// returns single bytes per component - assumes [0.0, 1.0] range
// clip or constrain out of range values before using
inline unsigned char rc( const frgb &c ) {  return (unsigned char)glut.linear_to_SRGB( c.R ); }
inline unsigned char gc( const frgb &c ) {  return (unsigned char)glut.linear_to_SRGB( c.G ); }
inline unsigned char bc( const frgb &c ) {  return (unsigned char)glut.linear_to_SRGB( c.B ); }

// inline unsigned int ul() {} // bit shifty stuff

// set component
inline void setr( frgb &c, const float& r )   { c.R = r; }
inline void setg( frgb &c, const float& g )   { c.G = g; }
inline void setb( frgb &c, const float& b )   { c.B = b; }
void set(  frgb &c, const float& r, const float& g, const float& b) { c.R = r;  c.G = g;  c.B = b; }
frgb set(  const float& r, const float& g, const float& b ) { return { r, g, b }; }

// TODO - set from bracketed list

inline void setrc( frgb &c, const unsigned char& r ) { c.R = glut.SRGB_to_linear( r ); }
inline void setgc( frgb &c, const unsigned char& g ) { c.G = glut.SRGB_to_linear( g ); }
inline void setbc( frgb &c, const unsigned char& b ) { c.B = glut.SRGB_to_linear( b ); }
void setc(  frgb &c, const unsigned char& r, const unsigned char& g, const unsigned char& b ) 
{ setrc( c, r );   setgc( c, g );   setbc( c, b ); }
frgb setc( const unsigned char& r, const unsigned char& g, const unsigned char& b )
{ return { glut.SRGB_to_linear( r ), glut.SRGB_to_linear( g ), glut.SRGB_to_linear( b ) }; }

//inline void setul( unsigned int in ) {} // bit shifty stuff

// Clip to range [ 0.0, 1.0 ] but keep colors in proportion
frgb& constrain( const frgb &c ) {
    frgb out = c;
    // find maximum color
    float max = linalg::maxelem( c );
    if( max > 1.0f ) {
        // bring negative colors to zero
        linalg::clamp( out, 0.0f, max );
        // if max color > 1.0 divide all colors by max
        out /= max;
    }
    return out;
}

/* std::ostream & operator << ( std::ostream &out, const frgb& c ) { 
        out << "Linear - R: " << c.R << " G: " << c.G << " B: " << c.B << "\n";
        return out;
}
*/

void print_SRGB( const frgb &c ) {
    int r = ( int )rc( c );
    int g = ( int )gc( c );
    int b = ( int )bc( c );
    std::cout << "SRGB   - R: " << r << " G: " << g << " B: " << b;
}
