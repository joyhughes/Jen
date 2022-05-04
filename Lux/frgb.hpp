#include <iostream>
#include <array>
#include <algorithm>

#define R 0
#define G 1
#define B 2

class frgb {
private:
    std::array< float, 3 > c;

public:
    // constructors
    frgb();
    frgb( float r, float g, float b );
    // TODO - add constructor from bracketed list

    // construct from 24 bit color
    frgb( unsigned char r, unsigned char g, unsigned char b );
    
    // might be more efficient to do conversion in image class as lookup table can be class member 
    // and conversion to a large block of frgb can use std::move
    // frgb( unsigned char r, unsigned char g, unsigned char b, std::unique_ptr<gamma_lut> t ); 
    // frgb( unsigned int in ) { bit shifty stuff}    
    // frgb( unsigned int in, std::unique_ptr<gamma_lut> t ) { bit shifty stuff }

    // destructor
    ~frgb();

    // iterators 

    // accessors
    // question - how to overload [] operator for assignment?
    float operator []( int i );

    inline float rf();
    inline float gf();
    inline float bf();

    // returns single bytes per component - assumes [0.0, 1.0] range
    // clip or constrain out of range values before using
    inline unsigned char rc();
    inline unsigned char gc();
    inline unsigned char bc();

    // inline unsigned int ul() {} // bit shifty stuff

    // set component
    inline void setrf( float r );
    inline void setgf( float g );
    inline void setbf( float b );
    inline void setf( float r, float g, float b);
    // TODO - set from bracketed list

    inline void setcr( unsigned char r );
    inline void setcg( unsigned char g );
    inline void setcb( unsigned char b );
    inline void setc( unsigned char r, unsigned char g, unsigned char b );

    inline void setul( unsigned int in );

    // arithmetic operators
    frgb& operator /= (const float rhs);

    // I/O operators
    friend std::ostream &operator << ( std::ostream &out, const frgb& f ) { 
        out << "R: " << f.c[ R ] << " G: " << f.c[ G ] << " B: " << f.c[ B ];
        return out;
    }

    // color clipping
    void clip( float minc, float maxc );
    // Clip to range [ 0.0, 1.0 ] but keep colors in proportion
    void constrain();
};