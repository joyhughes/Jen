#include "frgb.hpp"
#include "gamma_lut.hpp"

static gamma_LUT glut( 2.2f );

frgb::frgb()                              { c[ R ] = 0.0;     c[ G ] = 0.0;   c[ B ] = 0.0; }
frgb::frgb( float r, float g, float b )   { c[ R ] = r;       c[ G ] = g;     c[ B ]= b;    }
// TODO - add constructor from bracketed list

// construct from 24 bit color
frgb::frgb( unsigned char r, unsigned char g, unsigned char b ) { 
    c[ R ] = glut.SRGB_to_linear( r );
    c[ G ] = glut.SRGB_to_linear( g );
    c[ B ] = glut.SRGB_to_linear( b );
};

// frgb( unsigned int in ) { bit shifty stuff}    

// destructor
frgb::~frgb() {}

// iterators 

// accessors
// question - how to overload [] operator for assignment?
float& frgb::operator []( int i ) { return c[ i ]; }

inline float frgb::r()       { return c[ R ]; }
inline float frgb::g()       { return c[ G ]; }
inline float frgb::b()       { return c[ B ]; }

// returns single bytes per component - assumes [0.0, 1.0] range
// clip or constrain out of range values before using
inline unsigned char frgb::rc() {  return (unsigned char)glut.linear_to_SRGB( c[ R ] ); }
inline unsigned char frgb::gc() {  return (unsigned char)glut.linear_to_SRGB( c[ G ] ); }
inline unsigned char frgb::bc() {  return (unsigned char)glut.linear_to_SRGB( c[ B ] ); }

// inline unsigned int ul() {} // bit shifty stuff

// set component
inline void frgb::setr( float r )   { c[ R ] = r; }
inline void frgb::setg( float g )   { c[ G ] = g; }
inline void frgb::setb( float b )   { c[ B ] = b; }
inline void frgb::set( float r, float g, float b) { c[ R ] = r;  c[ G ] = g;  c[ B ] = b; }
// TODO - set from bracketed list

inline void frgb::setrc( unsigned char r ) { c[ R ] = glut.SRGB_to_linear( r ); }
inline void frgb::setgc( unsigned char g ) { c[ G ] = glut.SRGB_to_linear( g ); }
inline void frgb::setbc( unsigned char b ) { c[ B ] = glut.SRGB_to_linear( b ); }
inline void frgb::setc( unsigned char r, unsigned char g, unsigned char b ) { setrc( r );   setgc( g );   setbc( b ); }

//inline void frgb::setul( unsigned int in ) {} // bit shifty stuff

// arithmetic operators
const frgb& frgb::operator += ( frgb rhs ) { c[R] += rhs.c[R];  c[G] += rhs.c[G];  c[B] += rhs.c[B];  return *this; }
const frgb& frgb::operator -= ( frgb rhs ) { c[R] -= rhs.c[R];  c[G] -= rhs.c[G];  c[B] -= rhs.c[B];  return *this; }
const frgb& frgb::operator *= ( frgb rhs ) { c[R] *= rhs.c[R];  c[G] *= rhs.c[G];  c[B] *= rhs.c[B];  return *this; }

const frgb& frgb::operator *= ( float rhs ) { c[R] *= rhs;  c[G] *= rhs;  c[B] *= rhs;  return *this; }
const frgb& frgb::operator /= ( float rhs ) { c[R] /= rhs;  c[G] /= rhs;  c[B] /= rhs;  return *this; }


frgb frgb::operator + ( frgb rhs ) { 
    frgb out;  
    out.set( c[R] + rhs.c[R],  c[G] + rhs.c[G],  c[B] + rhs.c[B] ); 
    return out; 
}

frgb frgb::operator - ( frgb rhs ) { 
    frgb out;  
    out.set( c[R] - rhs.c[R],  c[G] - rhs.c[G],  c[B] - rhs.c[B] ); 
    return out; 
}

frgb frgb::operator * ( frgb rhs ) { 
    frgb out;  
    out.set( c[R] * rhs.c[R],  c[G] * rhs.c[G],  c[B] * rhs.c[B] ); 
    return out; 
}

frgb frgb::operator * ( float rhs ) { 
    frgb out;  
    out.set( c[R] * rhs,  c[G] * rhs,  c[B] * rhs ); 
    return out; 
}

frgb frgb::operator / ( float rhs ) { 
    frgb out;  
    out.set( c[R] / rhs,  c[G] / rhs,  c[B] / rhs ); 
    return out; 
}

// color clipping
void frgb::clamp( float minc, float maxc ) {
    c[ R ] = std::clamp( c[ R ], minc, maxc );
    c[ G ] = std::clamp( c[ G ], minc, maxc );
    c[ B ] = std::clamp( c[ B ], minc, maxc );
}

// Clip to range [ 0.0, 1.0 ] but keep colors in proportion
void frgb::constrain() {
    // find maximum color
    float max = *std::max_element( c.begin(), c.end() );
    if( max > 1.0f ) {
        // bring negative colors to zero
        clamp( 0.0f, max );
        // if max color > 1.0 divide all colors by max
        *this /= max;
    }
}

void frgb::print_SRGB() {
    int r = ( int )rc();
    int g = ( int )gc();
    int b = ( int )bc();
    std::cout << "SRGB   - R: " << r << " G: " << g << " B: " << b;
}


int main() {
    frgb color( -1.5f, 0.5f, 2.5f );
    std::cout << "Initial color     " << color << "\n\n";
    color.clamp( -1.0f, 2.0f );
    std::cout << "Clamped color     " << color << "\n\n";
    color.constrain();
    std::cout << "Constrained color " << color << "\n\n";
    color.print_SRGB();

    frgb ucolor( (unsigned char)0x00, (unsigned char)0x80, (unsigned char)0xff );
    std::cout << "\n\nColor from unsigned char " << ucolor << "\n\n";
    ucolor.print_SRGB();
    std::cout << "\n\n";

    ucolor += color;
    std::cout << "Color added with += " << ucolor << "\n\n";

    frgb acolor = ucolor + color;
    std::cout << "Color added with +  " << acolor << "\n\n";

    color[ 0 ] = 0.5;
    std::cout << "Color after color[ 0 ] = 0.5 " << color << "\n\n";

    float a = color[ 0 ];

    auto it = color.begin();

    return 0;
}
