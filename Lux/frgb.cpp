#include "frgb.hpp"
#include "srgb.hpp"

frgb::frgb()                              { c[ R ] = 0.0;     c[ G ] = 0.0;   c[ B ] = 0.0; }
frgb::frgb( float r, float g, float b )   { c[ R ] = r;       c[ G ] = g;     c[ B ]= b;    }
// TODO - add constructor from bracketed list

// construct from 24 bit color
frgb::frgb( unsigned char r, unsigned char g, unsigned char b ) { 
    c[ R ] = srgb::srgb_to_linear( r );
    c[ G ] = srgb::srgb_to_linear( g );
    c[ B ] = srgb::srgb_to_linear( b );
};

// might be more efficient to do conversion in image class as lookup table can be class member 
// and conversion to a large block of frgb can use std::move
//frgb::frgb( unsigned char r, unsigned char g, unsigned char b, std::unique_ptr<gamma_lut> t ) 
//    { c[ R ] = t->lookup( r );   c[ G ] = t->lookup( g );     c[ B ] = t->lookup( b ); }
// frgb( unsigned int in ) { bit shifty stuff}    
// frgb( unsigned int in, std::unique_ptr<gamma_lut> t ) { bit shifty stuff }

// destructor
frgb::~frgb() {}

// iterators 

// accessors
// question - how to overload [] operator for assignment?
float frgb::operator []( int i ) { return c[ i ]; }

inline float frgb::rf()       { return c[ R ]; }
inline float frgb::gf()       { return c[ G ]; }
inline float frgb::bf()       { return c[ B ]; }

// returns single bytes per component - assumes [0.0, 1.0] range
// clip or constrain out of range values before using
inline unsigned char frgb::rc() {  return (unsigned char)srgb::linear_to_srgb( c[ R ] ); }
inline unsigned char frgb::gc() {  return (unsigned char)srgb::linear_to_srgb( c[ G ] ); }
inline unsigned char frgb::bc() {  return (unsigned char)srgb::linear_to_srgb( c[ B ] ); }

// inline unsigned int ul() {} // bit shifty stuff

// set component
inline void frgb::setrf( float r )   { c[ R ] = r; }
inline void frgb::setgf( float g )   { c[ G ] = g; }
inline void frgb::setbf( float b )   { c[ B ] = b; }
inline void frgb::setf( float r, float g, float b) { c[ R ] = r;  c[ G ] = g;  c[ B ] = b; }
// TODO - set from bracketed list

inline void frgb::setcr( unsigned char r ) { c[ R ] = srgb::srgb_to_linear( r ); }
inline void frgb::setcg( unsigned char g ) { c[ G ] = srgb::srgb_to_linear( g ); }
inline void frgb::setcb( unsigned char b ) { c[ B ] = srgb::srgb_to_linear( b ); }
inline void frgb::setc( unsigned char r, unsigned char g, unsigned char b ) { setcr( r );   setcg( g );   setcb( b ); }

inline void frgb::setul( unsigned int in ) {} // bit shifty stuff

// arithmetic operators
frgb& frgb::operator /= (const float rhs) { c[R] /= rhs;  c[G] /= rhs;  c[B] /= rhs;  return *this; }

// color clipping
void frgb::clip( float minc, float maxc ) {
    using namespace std;
    frgb out;
    // SIMD friendly way of writing clamp()
    // https://stackoverflow.com/questions/45541921/c-clamp-function-for-a-stdvector
    transform( begin( c ), end( c ), begin( out.c ), [ maxc ]( float f ) { return min( f, maxc ); } );
    transform( begin( c ), end( c ), begin( out.c ), [ minc ]( float f ) { return max( f, minc ); } );
}

// Clip to range [ 0.0, 1.0 ] but keep colors in proportion
void frgb::constrain() {
    // find maximum color
    float max = *std::max_element( c.begin(), c.end() );
    if( max > 1.0f ) {
        // bring negative colors to zero
        clip( 0.0f, max );
        // if max color > 1.0 divide all colors by max
        *this /= max;
    }
}


int main() {
    frgb color( -1.5f, 0.5f, 2.5f );
    std::cout << "Initial color     " << color << "\n\n";
    color.clip( -1.0f, 2.0f );
    std::cout << "Clipped color     " << color << "\n\n";
    color.constrain();
    std::cout << "Constrained color " << color << "\n\n";

    frgb ucolor( (unsigned char)0x00, (unsigned char)0x80c, (unsigned char)0xffc );
    std::cout << "Unsigned char color " << ucolor << "\n\n";

    return 0;
}
