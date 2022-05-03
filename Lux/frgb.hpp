#include <iostream>
#include <array>
#include <algorithm>
#include "srgb.hpp"

#define R 0
#define G 1
#define B 2

class frgb {
private:
    std::array< float, 3 > c;

public:
    // constructors
    frgb()                              { c[ R ] = 0.0;     c[ G ] = 0.0;   c[ B ] = 0.0; }
    frgb( float r, float g, float b )   { c[ R ] = r;       c[ G ] = g;     c[ B ]= b;    }
    // TODO - add constructor from bracketed list

    // construct from 24 bit color
    frgb( unsigned char r, unsigned char g, unsigned char b ) { 
        c[ R ] = srgb::srgb_to_linear( r );
        c[ G ] = srgb::srgb_to_linear( g );
        c[ B ] = srgb::srgb_to_linear( b );
    };
    
    // might be more efficient to do conversion in image class as lookup table can be class member 
    // and conversion to a large block of frgb can use std::move
    //frgb( unsigned char r, unsigned char g, unsigned char b, std::unique_ptr<gamma_lut> t ) 
    //    { c[ R ] = t->lookup( r );   c[ G ] = t->lookup( g );     c[ B ] = t->lookup( b ); }
    // frgb( unsigned int in ) { bit shifty stuff}    
    // frgb( unsigned int in, std::unique_ptr<gamma_lut> t ) { bit shifty stuff }

    // destructor
    ~frgb() {}

    // iterators 

    // accessors
    // question - how to overload [] operator for assignment?
    float operator []( int i ) { return c[ i ]; }

    inline float rf()       { return c[ R ]; }
    inline float gf()       { return c[ G ]; }
    inline float bf()       { return c[ B ]; }

    // returns single bytes per component - assumes [0.0, 1.0] range
    // clip or constrain out of range values before using
    inline unsigned char rc() {  return (unsigned char)srgb::linear_to_srgb( c[ R ] ); }
    inline unsigned char gc() {  return (unsigned char)srgb::linear_to_srgb( c[ G ] ); }
    inline unsigned char bc() {  return (unsigned char)srgb::linear_to_srgb( c[ B ] ); }

    // inline unsigned int ul() {} // bit shifty stuff

    // set component
    inline void setrf( float r )   { c[ R ] = r; }
    inline void setgf( float g )   { c[ G ] = g; }
    inline void setbf( float b )   { c[ B ] = b; }
    inline void setf( float r, float g, float b) { c[ R ] = r;  c[ G ] = g;  c[ B ] = b; }
    // TODO - set from bracketed list

    inline void setcr( unsigned char r ) { c[ R ] = srgb::srgb_to_linear( r ); }
    inline void setcg( unsigned char g ) { c[ G ] = srgb::srgb_to_linear( g ); }
    inline void setcb( unsigned char b ) { c[ B ] = srgb::srgb_to_linear( b ); }
    inline void setc( unsigned char r, unsigned char g, unsigned char b ) { setcr( r );   setcg( g );   setcb( b ); }

    inline void setul( unsigned int in ) {} // bit shifty stuff

    // arithmetic operators
    frgb& operator /= (const float rhs) { c[R] /= rhs;  c[G] /= rhs;  c[B] /= rhs;  return *this; }

    // I/O operators
    friend std::ostream &operator << ( std::ostream &out, const frgb& f ) { 
        out << "R: " << f.c[ R ] << " G: " << f.c[ G ] << " B: " << f.c[ B ];
        return out;
    }

    // color clipping
    void clip( float min, float max ) {
        frgb out;
        // SIMD friendly way of writing clamp()
        // https://stackoverflow.com/questions/45541921/c-clamp-function-for-a-stdvector
        //transform( begin( c ), end( c ), begin( out.c ), [ maxc ]( float f ) { return min( f, maxc ); } );
        //transform( begin( c ), end( c ), begin( out.c ), [ maxc ]( float f ) { return max( f, minc ); } );

        c[ R ] = std::clamp( c[ R ], min, max );
        c[ G ] = std::clamp( c[ G ], min, max );
        c[ B ] = std::clamp( c[ B ], min, max );
    };

    // Clip to range [ 0.0, 1.0 ] but keep colors in proportion
    void constrain() {
        // find maximum color
        float max = *std::max_element( c.begin(), c.end() );
        if( max > 1.0f ) {
            // bring negative colors to zero
            clip( 0.0f, max );
            // if max color > 1.0 divide all colors by max
            *this /= max;
        }
    }
};

/*
class gamma_lut {
public:
    std::array< float, 256 > lut;
    float gamma;

    // constructor
    gamma_lut( float g ) {
        gamma = g;
        int i;
        // How to do calculation using srgb::linear_to_srgb() and std::transform() or similar?
        for ( i=0; i<256; i++ ) {
            lut[ i ] = srgb::srgb_to_linear( i );
        }
    }

    float lookup( unsigned char index ) { return lut[ index ]; }
    frgb  lookup( unsigned char r, unsigned char g, unsigned char b )
        { return frgb( lookup( r ), lookup( g ), lookup( b ) ); }
    //frgb lookup( unsigned int c ) { bit shifty stuff }

    // Reverse lookup? Quick tree search
}; 
*/